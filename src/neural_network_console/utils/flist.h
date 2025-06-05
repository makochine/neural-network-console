// Copyright 2025 Sony Group Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __ELFE_FLIST_H__
#define __ELFE_FLIST_H__

#include <stdio.h>	// FILE
#include <memory.h>	// memcpy
#include <cassert>
#include <memory>
#include <vector>
#include <algorithm>

namespace elfe{

	static const int MINIMUM_PREALLOC_SIZE  = 16;

	/*****************************************************************************/
	/* template list class FListT */
	template <typename _T> class FListT
	{
	public:
		FListT();											/** constructor */
		FListT(const FListT &src);							/** copy constructor */
		virtual ~FListT();									/** destructor */
	public:
		FListT& operator=(const FListT &src);				/** assign operator (copy) */
		int operator==(const FListT &src) const;
		int operator!=(const FListT &src) const {return !(*this == src);}
		_T &Add(const _T &item);							/** Add */
		void Add(const FListT<_T> &item_list);				/** Add */
		_T &AddSort(const _T &item);						/** AddSort */
		_T &SetAt(int index, const _T &item);				/** SetAt */
		_T &Insert(int index, const _T &item);				/** Insert */
		void Exchange(int index1, int index2);				/** Exchange */
		int GetSize() const;								/** Get List size */
		void Delete(int index);								/** Delete */
		void Remove(const _T &item);						/** Delete */
		const _T &GetAt(int index) const;							/** Reference */
		const _T* GetRawBuffer() const;						/** Get as raw buffer */
		void Clear();										/** All Clear */
		int IndexOf(const _T &item) const;					/** search index */
		int IndexToInsert(const _T &item) const;			/** index to insert */
		void Sort();										/** sort (calls QuickSort) */
		template <typename _T2> void SortWith(FListT<_T2> &list);/** sort (calls QuickSort) */
		void SortAndDeleteDuplex();							/** sort and delete duplication */
		void Reverse();										/** reverse orders */
		void Copy(const FListT &src);						/** copy */
		void Append(const FListT &src);						/** append copy */
		void SetSorted(int sorted) {sorted_ = sorted;};		/** Set Sorted */
		int GetSorted() const {return sorted_;};			/** Get Sorted */
		void SaveToFile(FILE *F) const;						/** Save to file */
		void LoadFromFile(FILE *F);							/** Load from file */
		void Swap(int index1, int index2);					/** Swap Item */
		_T Mean() const;
		_T Variance(_T mean) const;
		_T Min() const;
		_T Max() const;
	private:
		void Alloc(int size);								/** pre-alloc memory */
		void Twice();										/** double size pre-alloc memory */
		void Half();										/** half size pre-alloc memory */
	private:
		bool sorted_;
		std::vector<_T> items_;								/** internal items */
	};

	/*****************************************************************************/
	/** constructor */
	template <typename _T> FListT<_T>::FListT() :
		sorted_(false),
		items_()
	{
	}

	/*****************************************************************************/
	/** copy constructor */
	template <typename _T> FListT<_T>::FListT(const FListT &src) :
		sorted_(src.sorted_),
		items_(src.items_)
	{
	}

	/*****************************************************************************/
	/** destructor */
	template <typename _T> FListT<_T>::~FListT()
	{
		Clear();
	}

	/*****************************************************************************/
	/** assign operator (copy) */
	template <typename _T> FListT<_T> &FListT<_T>::operator=(const FListT &src)
	{
		assert(&src != this);

		items_ = src.items_;
		sorted_ = src.sorted_;
		return *this;
	}

	/*****************************************************************************/
	/** compare operator (equal) */
	template <typename _T> int FListT<_T>::operator==(const FListT &src) const
	{
		return items_ == src.items_;
	}

	/*****************************************************************************/
	/** Copy */
	template <typename _T> void FListT<_T>::Copy(const FListT &src)
	{
		assert(&src != this);

		items_ = src.items_;
		sorted_ = src.sorted_;
	}

	/*****************************************************************************/
	/** Append */
	template <typename _T> void FListT<_T>::Append(const FListT &src)
	{
		assert(&src != this);

		sorted_ = false;
		for (const auto& item: src.items_) {
			items_.push_back(item);
		}
	}

	/*****************************************************************************/
	/** Save to file */
	template <typename _T> void FListT<_T>::SaveToFile(FILE *F) const
	{
		int size = (int)items_.size();
		fwrite(&size, sizeof(size), 1, F);
		fwrite(items_.data(), sizeof(_T), size, F);
	}
	/*****************************************************************************/
	/** Load from file */
	template <typename _T> void FListT<_T>::LoadFromFile(FILE *F)
	{
		int size;
		Clear();

		fread(&size, sizeof(size), 1, F);
		items_.resize(size);
		fread(items_.data(), sizeof(_T), size, F);
	}
	/*****************************************************************************/
	/** Swap Item */
	template <typename _T> void FListT<_T>::Swap(int index1, int index2)
	{
		assert (index1 < (int)items_.size());
		assert (index2 < (int)items_.size());
		std::swap(items_[index1], items_[index2]);
	}
	/*****************************************************************************/
	/** Calc mean */
	template <typename _T> _T FListT<_T>::Mean() const
	{
		assert(GetSize() != 0);
		_T sum = (_T)0;
		for (int i = 0 ; i < GetSize() ; i++){
			sum += GetAt(i);
		}
		return sum / GetSize();
	}
	/*****************************************************************************/
	/** Calc variance */
	template <typename _T> _T FListT<_T>::Variance(_T mean) const
	{
		assert(GetSize() != 0);
		_T var_sum = (_T)0;
		for (int i = 0 ; i < GetSize() ; i++){
			const _T v = GetAt(i) - mean;
			var_sum += v * v;
		}
		return var_sum / GetSize();
	}
	/*****************************************************************************/
	/** Calc min */
	template <typename _T> _T FListT<_T>::Min() const
	{
		assert(GetSize() != 0);
		_T min = GetAt(0);
		for (int i = 1 ; i < GetSize() ; i++){
			if (GetAt(i) < min) {
				min = GetAt(i);
			}
		}
		return min;
	}
	/*****************************************************************************/
	/** Calc max */
	template <typename _T> _T FListT<_T>::Max() const
	{
		assert(GetSize() != 0);
		_T max = GetAt(0);
		for (int i = 1 ; i < GetSize() ; i++){
			if (GetAt(i) > max) {
				max = GetAt(i);
			}
		}
		return max;
	}
	/*****************************************************************************/
	/** pre-alloc memory */
	template <typename _T> void FListT<_T>::Alloc(int size)
	{
		Clear();
		items_.resize(size);
	}
	/*****************************************************************************/
	/** double size pre-alloc memory */
	template <typename _T> void FListT<_T>::Twice()
	{
		size_t new_size;
		if (0 < items_.size()) {
			new_size = items_.size() * 2;
		}
		else {
			new_size = MINIMUM_PREALLOC_SIZE;
		}
		items_.resize(new_size);
	}

	/*****************************************************************************/
	/** half size pre-alloc memory */
	template <typename _T> void FListT<_T>::Half()
	{
		size_t new_size = items_.size() / 2;
		items_.resize(new_size);
	}

	/*****************************************************************************/
	/** Get list size */
	template <typename _T> int FListT<_T>::GetSize() const
	{
		return (int)items_.size();
	}

	/*****************************************************************************/
	/** Add */
	template <typename _T> _T &FListT<_T>::Add(const _T &item)
	{
		sorted_ = false;
		items_.push_back(item);
		return items_.back();
	}
	/*****************************************************************************/
	/** AddSort */
	template <typename _T> _T &FListT<_T>::AddSort(const _T &item)
	{
		bool sorted_bak = sorted_;
		_T &new_data = Insert(IndexToInsert(item), item);
		sorted_ = sorted_bak;
		return new_data;
	}
	/*****************************************************************************/
	/** Add */
	template <typename _T> void FListT<_T>::Add(const FListT<_T> &item_list)
	{
		assert(&item_list != this);

		for (const auto& item: item_list.items_) {
			items_.push_back(item);
		}
	}
	/*****************************************************************************/
	/** SetAt */
	template <typename _T> _T &FListT<_T>::SetAt(int index, const _T &item)
	{
		assert(0 <= index);
		assert(index < (int)items_.size());

		sorted_ = false;
		items_[index] = item;
		return items_[index];
	}

	/*****************************************************************************/
	/** Insert */
	template <typename _T> _T &FListT<_T>::Insert(int index, const _T &item)
	{
		sorted_ = false;
		items_.insert(items_.begin() + index, item);
		return items_[index];
	}

	/*****************************************************************************/
	/** Exchange */
	template <typename _T> void FListT<_T>::Exchange(int index1, int index2)
	{
		assert (index1 < (int)items_.size());
		assert (index2 < (int)items_.size());
		std::swap(items_[index1], items_[index2]);
	}

	/*****************************************************************************/
	/** Delete */
	template <typename _T> void FListT<_T>::Delete(int index)
	{
		assert (index < (int)items_.size());
		items_.erase(items_.begin() + index);
	}

	/*****************************************************************************/
	/** Delete */
	template <typename _T> void FListT<_T>::Remove(const _T &item)
	{
		int index = IndexOf(item);
		if (index >= 0) {
			Delete(index);
		}
	}

	/*****************************************************************************/
	/** Get Reference */
	template <typename _T> const _T &FListT<_T>::GetAt(int index) const
	{
		assert (0 <= index);
		assert (index < (int)items_.size());

		return items_[index];
	}

	/*****************************************************************************/
	/** Get Raw Buffer */
	template <typename _T> const _T* FListT<_T>::GetRawBuffer() const
	{
		return items_.data();
	}

	/*****************************************************************************/
	/** Clear All */
	template <typename _T> void FListT<_T>::Clear()
	{
		items_.clear();
		sorted_ = false;
	}

	/*****************************************************************************/
	/** Search Index */
	template <typename _T> int FListT<_T>::IndexOf(const _T &item) const
	{
		size_t index = -1;
		//itemのリスト中のIndexを取得
		if (!sorted_) {
			auto it = std::find(items_.begin(), items_.end(), item);
			if (it != items_.end()) {
				index = std::distance(items_.begin(), it);
			}
		}else {
			//Binary search
			if (std::binary_search(items_.begin(), items_.end(), item)) {
				auto it = std::lower_bound(items_.begin(), items_.end(), item);
				index = std::distance(items_.begin(), it);
			}
		}
		return (int)index;
	}
	/*****************************************************************************/
	/** Search Index to add */
	template <typename _T> int FListT<_T>::IndexToInsert(const _T &item) const
	{
		size_t index = items_.size();

		if (!sorted_) {
			for ( size_t i = 0; i < items_.size(); ++i ) {
				if ( items_[i] > item ) {
					index = i;
					break;
				}
			}
		} else {
			//Binary search
			auto it = std::lower_bound(items_.begin(), items_.end(), item);
			index = std::distance(items_.begin(), it);
		}
		return (int)index;
	}

	/*****************************************************************************/
	/** Sort */
	template <typename _T> void FListT<_T>::Sort()
	{
		std::sort(items_.begin(), items_.end());
		sorted_ = true;
	}

	/*****************************************************************************/
	/** Sort and delete duplication */
	template <typename _T> void FListT<_T>::SortAndDeleteDuplex()
	{
		Sort();
		auto it = std::unique(items_.begin(), items_.end());
		items_.erase(it, items_.end());
	}

	/*****************************************************************************/
	/** Reverse Orders */
	template <typename _T> void FListT<_T>::Reverse()
	{
		sorted_ = false;
		std::reverse(items_.begin(), items_.end());
	}

	/*****************************************************************************/
	/* template pointer list class FListP */
	template <typename _T> class FListP
	{
	public:
		FListP();											/** constructor */
		FListP(const FListP &src);							/** copy constructor */
		virtual ~FListP();									/** destructor */
	public:
		FListP& operator=(const FListP &src);				/** assign operator */
		int operator==(const FListP &src) const;			/** Is same */
		int operator!=(const FListP &src) const {return !(*this == src);}
		_T &Add();											/** Add */
		_T &Add(const _T &item);							/** Add */
		void Add(const FListP<_T> &item_list);				/** Add */
		_T &SetAt(int index, const _T &item);				/** SetAt */
		_T &Insert(int index);								/** Insert */
		_T &Insert(int index, const _T &item);				/** Insert */
		void Exchange(int index1, int index2);				/** Exchange */
		int GetSize() const;								/** Get list size */
		void Delete(int index);								/** Delete */
		void Remove(const _T &item);						/** Delete */
		_T &GetAt(int index) const;							/** Item Reference */
		void Clear();										/** Clear All */
		int IndexOf(const _T &I) const;						/** Search index */
		void Sort();										/** Sort (call QuickSort) */
		void SortAndDeleteDuplex();							/** Sort and deleete duplication */
		void Reverse();										/** Reverse Orders */
		void Shuffle();										/** Shuffle */
		void Copy(const FListP &src);						/** copy */
		void Append(const FListP &src);						/** append copy */
		void SetSorted(int sorted) {sorted_ = sorted;};		/** Set Sorted */
		int GetSorted() const {return sorted_;};			/** Get Sorted */
		void SaveToFile_Struct(FILE *F) const;				/** Save to file */
		void LoadFromFile_Struct(FILE *F);					/** Load from file */
		void Swap(int index1, int index2);					/** Swap Item */
	private:
		bool sorted_;
		std::vector<_T*> items_;
	};

	/*****************************************************************************/
	/** constructor */
	template <typename _T> FListP<_T>::FListP() :
		sorted_(false),
		items_()
	{
	}

	/*****************************************************************************/
	/** copy constructor */
	template <typename _T> FListP<_T>::FListP(const FListP &src)
	{
		// for (_T* itemptr : src.items_) {
		// 	items_.emplace_back(new _T(*itemptr));
		// }
		for ( int i = 0; i < src.GetSize(); ++i ) {
			Add(src.GetAt(i));
		}
		sorted_ = src.sorted_;
	}

	/*****************************************************************************/
	/** destructor */
	template <typename _T> FListP<_T>::~FListP()
	{
		Clear();
	}

	/*****************************************************************************/
	/** assign operator */
	template <typename _T> FListP<_T> &FListP<_T>::operator=(const FListP &src)
	{
		Copy(src);
		return *this;
	}

	/*****************************************************************************/
	/** Is same */
	template <typename _T> int FListP<_T>::operator==(const FListP &src) const
	{
		if ( GetSize() != src.GetSize() ){
			return false;
		}
		for ( int i = 0 ; i < GetSize() ; ++i ) {
			if ( GetAt(i) != src.GetAt(i) ) {
				return false;
			}
		}
		return true;
	}

	/*****************************************************************************/
	/** Copy */
	template <typename _T> void FListP<_T>::Copy(const FListP &src)
	{
		Clear();
		// for (_T* itemptr : src.items_) {
		// 	items_.emplace_back(new _T(*itemptr));
		// }
		for ( int i = 0; i < src.GetSize(); ++i ) {
			Add(src.GetAt(i));
		}
		sorted_ = src.sorted_;
	}

	/*****************************************************************************/
	/** Append */
	template <typename _T> void FListP<_T>::Append(const FListP &src)
	{
		Add(src);
	}

	/*****************************************************************************/
	/** Save to file */
	template <typename _T> void FListP<_T>::SaveToFile_Struct(FILE *F) const
	{
		int size = (int)items_.size();
		fwrite(&size, sizeof(int), 1, F);
		for (size_t i = 0 ; i < items_.size(); i++) {
			fwrite(items_[i], sizeof(_T), 1, F);
		}
	}

	/*****************************************************************************/
	/** Load from file */
	template <typename _T> void FListP<_T>::LoadFromFile_Struct(FILE *F)
	{
		int size;
		Clear();
		fread(&size, sizeof(int), 1, F);
		for (int i = 0 ; i < size ; i++) {
			_T* pointer = new _T;
			fread(pointer, sizeof(_T), 1, F);
			items_.emplace_back(pointer);
		}
	}
	/*****************************************************************************/
	/** Swap Item */
	template <typename _T> void FListP<_T>::Swap(int index1, int index2)
	{
		sorted_ = false;
		std::swap(items_[index1], items_[index2]);
	}
	/*****************************************************************************/
	/** Get list size */
	template <typename _T> int FListP<_T>::GetSize() const
	{
		return (int)items_.size();
	}

	/*****************************************************************************/
	/** Add */
	template <typename _T> _T &FListP<_T>::Add()
	{
		sorted_ = false;
		items_.emplace_back(new _T);
		return *items_.back();
	}
	/*****************************************************************************/
	/** Add */
	template <typename _T> _T &FListP<_T>::Add(const _T &item)
	{
		_T* newitem = &Add();
		*newitem = item;
		return *newitem;
		// items_.emplace_back(new _T(item));
		// return *items_.back();
	}
	/*****************************************************************************/
	/** Add */
	template <typename _T> void FListP<_T>::Add(const FListP<_T> &item_list)
	{
		assert(&item_list != this);

		sorted_ = false;
		for (int i = 0; i < item_list.GetSize() ; i++) {
			Add(item_list.GetAt(i));
		}
		// for (_T* itemptr : item_list.items_) {
		// 	items_.emplace_back(new _T(*itemptr));
		// }
	}

	/*****************************************************************************/
	/** SetAt */
	template <typename _T> _T &FListP<_T>::SetAt(int index, const _T &item)
	{
		assert(0 <= index);
		assert(index < (int)items_.size());

		sorted_ = false;
		*items_[index] = item;
		return *items_[index];
	}

	/*****************************************************************************/
	/** Insert */
	template <typename _T> _T &FListP<_T>::Insert(int index)
	{
		assert(0 <= index);
		assert(index <= (int)items_.size());

		sorted_ = false;
		_T* newitem = new _T;
		items_.insert(items_.begin() + index, newitem);
		return *newitem;
	}

	/*****************************************************************************/
	/** Insert */
	template <typename _T> _T &FListP<_T>::Insert(int index, const _T &item)
	{
		assert(0 <= index);
		assert(index <= (int)items_.size());

		sorted_ = false;
		_T* newitem = new _T;
		*newitem = item;
		items_.insert(items_.begin() + index, newitem);
		return *newitem;
	}

	/*****************************************************************************/
	/** Exchange */
	template <typename _T> void FListP<_T>::Exchange(int index1, int index2)
	{
		sorted_ = false;
		std::swap(items_[index1], items_[index2]);
	}

	/*****************************************************************************/
	/** Delete */
	template <typename _T> void FListP<_T>::Delete(int index)
	{
		if ((index < 0) || (GetSize() <= index)) {
			return;
		}
		delete items_[index];
		items_.erase(items_.begin() + index);
	}

	/*****************************************************************************/
	/** Delete */
	template <typename _T> void FListP<_T>::Remove(const _T &item)
	{
		for (auto it = items_.begin(); it != items_.end(); ++it) {
			if (**it == item) {
				delete *it;
				it = items_.erase(it);
				break;
			}
		}
	}

	/*****************************************************************************/
	/** get Reference */
	template <typename _T> _T &FListP<_T>::GetAt(int index) const
	{
		assert(0 <= index);
		assert(index < (int)items_.size());
		/*
		if (!(index >= 0 && index < pointer_size_)) {
			index = index;
		}*/
		// assert(index >= 0 && index < pointer_size_);
		return *items_[index];
	}

	/*****************************************************************************/
	/** Clear All */
	template <typename _T> void FListP<_T>::Clear()
	{
		for (auto it = items_.begin(); it != items_.end(); ++it) {
			delete *it;
		}
		items_.clear();
		sorted_ = false;
	}

	/*****************************************************************************/
	/** Search Index */
	template <typename _T> int FListP<_T>::IndexOf(const _T &item) const
	{
		size_t index = -1;

		//itemのリスト中のIndexを取得
		if (!sorted_) {
			for (auto it = items_.begin(); it != items_.end(); ++it) {
				if (**it == item) {
					index = std::distance(items_.begin(), it);
					break;
				}
			}
		}else {
			//Binary search
			bool is_exist = binary_search(
				items_.begin(),
				items_.end(),
				&item,
				[](const _T* a, const _T* b) { return *a < *b; }
			);
			if (is_exist) {
				auto it = std::lower_bound(
					items_.begin(),
					items_.end(),
					&item,
					[](const _T* a, const _T* b) { return *a < *b; }
				);
				if (it != items_.end()) {
					index = std::distance(items_.begin(), it);
				}
			}
		}
		return (int)index;
	}

	/*****************************************************************************/
	/** Sort (call QuickSort) */
	template <typename _T> void FListP<_T>::Sort()
	{
		std::sort(
			items_.begin(),
			items_.end(),
			[](const _T* a, const _T* b) { return *a < *b; }
		);
		sorted_ = true;
	}

	/*****************************************************************************/
	/** Sort and delete duplication */
	template <typename _T> void FListP<_T>::SortAndDeleteDuplex()
	{
		Sort();
		int pos = (int)items_.size() - 1;
		while (pos > 0) {
			if (*items_[pos] == *items_[pos - 1]) {
				Delete(pos);
			}
			--pos;
		}
	}

	/*****************************************************************************/
	/** Reverse Orders */
	template <typename _T> void FListP<_T>::Reverse()
	{
		sorted_ = false;
		std::reverse(items_.begin(), items_.end());
	}
}

#endif	//__ELFE_FLIST_H__

