/*
 * Copyright (c) 2026, Aidan <JcbbcEnjoyer>
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 *
*/

#pragma once
#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

template <typename T>
struct ScratchBuffer {
public:
	struct Lease {
	public:
		Lease(const Lease&) = delete;
		Lease& operator=(const Lease&) = delete;
		Lease& operator=(Lease&&) = delete;
		Lease(Lease&& o) noexcept : owner_(std::exchange(o.owner_, nullptr)), own_(std::move(o.own_)) {}

		~Lease() {
			if (owner_)
				owner_->Release();
		}

		std::vector<T>& operator*() {
			return owner_ ? owner_->data : own_;
		}
		std::vector<T>* operator->() {
			return &**this;
		}

	private:
		friend ScratchBuffer;
		explicit Lease(ScratchBuffer* owner) : owner_(owner) {}  // borrow shared
		explicit Lease(std::vector<T> v) : own_(std::move(v)) {} // own a fresh one

		ScratchBuffer* owner_ = nullptr; // non-null => borrowing the shared buffer
		std::vector<T> own_;             // only used when owner_ is null
	};

	explicit ScratchBuffer(std::size_t n, const T& init = T{}) : data(n, init), defaultSize_(n), init_(init) {}

	ScratchBuffer(std::initializer_list<T> il) : data(il), defaultSize_(il.size()) {}

	// Shared buffer if free, otherwise a new vector
	Lease Get() {
		if (inUse_)
			return Lease(std::vector<T>(defaultSize_, init_));
		inUse_ = true;
		return Lease(this);
	}

	std::vector<T> data;

private:
	void Release() {
		inUse_ = false;
		data.resize(defaultSize_);
	}

	bool inUse_ = false;
	std::size_t defaultSize_ = 0;
	T init_{};
};