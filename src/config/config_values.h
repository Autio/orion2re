#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace config {

    // Dense while capturing native defaults, then an immutable baseline with
    // independently owned edits. Copies of a frozen store copy only its edits.
    template <typename T>
    class s_config_values {
      public:
        s_config_values() = default;
        s_config_values(const s_config_values& other)
            : baseline_(other.baseline_),
              size_(other.size_), baseline_size_(other.baseline_size_) {
            if (other.dense_) dense_ = std::make_unique<std::vector<T>>(*other.dense_);
            if (other.edits_) {
                edits_ = std::make_unique<t_edits>();
                for (const auto& edit : *other.edits_) {
                    edits_->emplace(edit.first, std::make_unique<T>(*edit.second));
                }
            }
        }
        s_config_values(s_config_values&&) noexcept = default;
        s_config_values& operator=(s_config_values&&) noexcept = default;
        s_config_values& operator=(const s_config_values& other) {
            if (this != &other) *this = s_config_values(other);
            return *this;
        }

        size_t size() const {
            return baseline_ ? size_ : (dense_ ? dense_->size() : 0);
        }

        const T& Read(size_t index) const {
            assert(index < size());
            if (!baseline_) return (*dense_)[index];
            if (edits_) {
                const auto found = edits_->find(index);
                if (found != edits_->end()) return *found->second;
            }
            if (index < baseline_size_) return (*baseline_)[index];
            static const T empty{};
            return empty;
        }

        const T& operator[](size_t index) const {
            return Read(index);
        }

        T& operator[](size_t index) {
            assert(index < size());
            if (!baseline_) return (*dense_)[index];
            if (!edits_) edits_ = std::make_unique<t_edits>();
            const auto found = edits_->find(index);
            if (found != edits_->end()) return *found->second;
            return *edits_->emplace(index, std::make_unique<T>(Read(index))).first->second;
        }

        void clear() {
            baseline_.reset();
            dense_.reset();
            edits_.reset();
            size_ = 0;
            baseline_size_ = 0;
        }

        void resize(size_t size) {
            if (!baseline_) {
                if (!dense_) dense_ = std::make_unique<std::vector<T>>();
                dense_->resize(size);
                return;
            }
            if (size < size_) {
                if (edits_) {
                    for (auto edit = edits_->begin(); edit != edits_->end();) {
                        if (edit->first >= size)
                            edit = edits_->erase(edit);
                        else
                            ++edit;
                    }
                }
                // Regrowth must value-initialize truncated elements, not expose
                // the old baseline contents again.
                baseline_size_ = std::min(baseline_size_, size);
            }
            size_ = size;
        }

        void Freeze() {
            assert(!baseline_);
            size_ = size();
            baseline_size_ = size_;
            if (!dense_) dense_ = std::make_unique<std::vector<T>>();
            baseline_ = std::shared_ptr<const std::vector<T>>(std::move(dense_));
        }

        const T* Baseline_Value(size_t index) const {
            return baseline_ && index < baseline_size_ ? &(*baseline_)[index] : nullptr;
        }

        void Reset(size_t index) {
            assert(Baseline_Value(index) != nullptr);
            if (edits_) {
                edits_->erase(index);
                if (edits_->empty()) edits_.reset();
            }
        }

        size_t Stored_Value_Count() const {
            return baseline_ ? (edits_ ? edits_->size() : 0) : size();
        }

        bool Shares_Baseline(const s_config_values& other) const {
            return baseline_ && baseline_ == other.baseline_ && baseline_size_ == other.baseline_size_;
        }

        template <typename Visitor>
        bool Visit_Stored(Visitor visitor) const {
            if (baseline_) {
                if (edits_) {
                    for (const auto& edit : *edits_) {
                        if (!visitor(edit.first, *edit.second)) return false;
                    }
                }
            } else {
                for (size_t index = 0; index < size(); ++index) {
                    if (!visitor(index, (*dense_)[index])) return false;
                }
            }
            return true;
        }

        void assign(size_t size, const T& value) {
            clear();
            dense_ = std::make_unique<std::vector<T>>(size, value);
        }

        void Initialize_Empty(size_t size) {
            clear();
            Freeze();
            resize(size);
        }

        template <typename Predicate>
        void Erase_Stored_If(Predicate predicate) {
            assert(baseline_);
            if (!edits_) return;
            for (auto edit = edits_->begin(); edit != edits_->end();) {
                if (predicate(edit->first))
                    edit = edits_->erase(edit);
                else
                    ++edit;
            }
        }

        void pop_back() {
            resize(size() - 1);
        }

        struct const_iterator {
            const s_config_values* owner;
            size_t index;
            const T& operator*() const {
                return owner->Read(index);
            }
            const_iterator& operator++() {
                ++index;
                return *this;
            }
            bool operator!=(const const_iterator& other) const {
                return index != other.index;
            }
        };
        const_iterator begin() const {
            return {this, 0};
        }
        const_iterator end() const {
            return {this, size()};
        }

      private:
        std::unique_ptr<std::vector<T>> dense_;
        std::shared_ptr<const std::vector<T>> baseline_;
        // Indirection permits recursive value types on all supported STL implementations.
        using t_edits = std::unordered_map<size_t, std::unique_ptr<T>>;
        std::unique_ptr<t_edits> edits_;
        size_t size_ = 0;
        size_t baseline_size_ = 0;
    };

} // namespace config