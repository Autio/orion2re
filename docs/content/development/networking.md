# Networking

The restored networking stack replaces the Windows-only DirectPlay path with a project-owned router protocol. It is designed for cross-platform multiplayer, one common LAN and online transport model, and a much smaller reliability layer.

The current design favors explicit session and disconnect behavior over transport-level faithfulness to the original implementation.

## Architecture

Each game process opens one TCP connection to a routing service. The router manages network identities, sessions, membership, and packet delivery. Game payloads remain opaque to it.

The roles are deliberately separate:

- the router owns transport, membership, and router-assigned network ids
- the session owner controls whether the session accepts new members
- the gameplay host owns turn progression and authoritative game-state updates
- game code owns player slots, disconnect policy, and AI takeover

The session owner and gameplay host commonly begin as the same process, but they are not the same architectural role. The router does not interpret turn, diplomacy, combat, or savegame data and must not elect a gameplay host.

The original DirectPlay wrapper and custom ACK, burst, fragmentation, and sender-queue transport are no longer part of the active path. TCP supplies reliable ordered delivery; the project protocol supplies framing and routing.

## Protocol framing

TCP is a byte stream, so every router message has an explicit length prefix and routing envelope. A frame contains:

- a 4-byte little-endian frame length
- a fixed protocol header containing the message kind, flags, session, sender, and target information
- an optional opaque payload

The receiver supports partial reads and multiple frames arriving in one read. Frame and payload sizes are bounded before allocation. Large payloads may be compressed with LZ4, but compression does not change the game-level packet identity.

The router derives sender and session identity from the authenticated connection state. It does not trust a client to claim another sender id or route through a session that the connection has not joined.

## Control and data planes

Control messages cover:

- hello and router identity
- session creation, listing, joining, leaving, opening, and closing
- member lists and join or leave notifications
- session closure and protocol errors

Data messages cover direct and broadcast delivery. The routing envelope is owned by the networking layer; the bytes inside it continue to use the game's packet identifiers and layouts where practical.

This boundary allows `NETMOX`, `NETTURN`, combat, diplomacy, and chat to keep their game-level packet model without preserving DirectPlay or the old reliable transport packets.

## Discovery and sessions

LAN discovery uses UDP broadcast to locate routers, not individual games. A client then connects to each discovered router over TCP and requests its session list. The browser merges those results and presents sessions to the player.

Online bootstrap can use a known endpoint, manual address, invite, or a future directory service. After the router endpoint is known, listing and joining use the same TCP protocol as LAN play.

One router session represents one pending or active game. A session remains routable while it has connected members. If its original owner leaves, remaining members can continue exchanging packets, but the ownerless session is not implicitly assigned to another process and normally does not reopen for new joins.

This matches the game's existing limitation: a player who leaves an in-progress game cannot transparently rejoin that same running game.

## Standalone and embedded routers

The router core runs as a single-threaded libuv event loop. It owns its sockets, connection state, session tables, timers, and routing decisions on that loop.

It can run in two forms:

- `orion2re-router` runs the blocking service loop as a standalone application.
- LAN hosting can start the same service on an SDL-managed worker thread inside the game process.

The embedded wrapper owns start, stop, and join behavior. It does not make the router core internally multithreaded. The game-side client also uses libuv and communicates with the router through the protocol rather than reaching into router state.

## Gameplay integration

The game keeps a mapping between router network ids and game player indexes. Direct sends name one router target; broadcasts fan out to the other members of the current session according to the protocol flags.

Higher layers still decide:

- which game packet should be sent
- whether an update is host-authoritative
- when a turn or combat phase is synchronized
- how a network member maps to an empire
- whether a disconnected empire becomes AI

The client layer reports membership events but does not manufacture game-state changes.

## Network debug logging

Add this setting to `orion2re.cfg` on both peers:

```text
interface net_debug_enabled = true;
```

Logging defaults to `false` in all builds. It is local to each peer, excluded
from saves and network synchronization, and takes effect on config load or
reload. Set it to `false` to disable logging.

SDL logs are written beside the executable in `logs/game.<pid>.log`.
Search for `netdbg:`. Packet logs include numeric packet ID, byte count,
sender/recipient network ID, resolved player slot, sync state, loopback,
broadcast skipping, and send result. A successful send means the transport
accepted the message, not that the peer processed it. Lua waits log their
membership and report missing player slots once per second and at timeout.
No packet payloads are dumped.

## Disconnect and host recovery

Voluntary departure can send the normal game-level dropout packet before leaving. An abrupt TCP disconnect cannot.

When the router reports that a member left, each remaining game maps the departed network id to a player and removes it from the connected-human candidates. If the gameplay host was lost, the clients independently choose the same recovery coordinator using shared game state. The current rule selects the first remaining connected game player. Only that coordinator broadcasts the game-level dropout and AI-takeover state; every peer applies the normal decode path.

This election is game-owned and does not require a final packet from the lost host. Router membership only supplies the fact that a connection disappeared.

If the router itself is lost, the session is lost. Transparent reconnect and router migration are not implemented. Recovery requires an explicit new session, normally from a synchronized save.

## Large state transfers

TCP framing removes the old transport's packet bursts and ACK-driven reassembly. The current game can send large framed payloads and retains some game-level differential or grouped formats used by world-state and combat synchronization.

Those formats can be simplified independently of the router. Keeping them initially reduces gameplay integration risk; replacing them later does not require another transport redesign.

## Source layout

- `src/network/router_protocol.cpp` implements encoding, decoding, validation, and framing.
- `src/network/router_core.cpp` implements session membership and routing decisions.
- `src/network/router_service.cpp` implements the libuv TCP and UDP service.
- `src/network/router_client.cpp` implements one client connection and queued protocol events.
- `src/network/router_session_browser.cpp` implements LAN discovery and session browsing.
- `src/game/netadapter.cpp` adapts router operations to the legacy game-facing networking calls and owns embedded-router lifecycle.
- `src/game/netcode.cpp`, `src/game/netmox.cpp`, and `src/game/netturn.cpp` own gameplay integration, disconnect recovery, and turn synchronization.

When changing the stack, keep the router packet-opaque, validate all lengths and connection-owned identities, and keep gameplay authority decisions out of the transport layer.