#pragma once

#ifndef _MEMPOOL_HPP_
#define _MEMPOOL_HPP_

#include <assert.h>
#include <stdint.h>
#include <stddef.h>


namespace MemPool
{

	class CDLink
	{
	public:
		struct DLNODE {
			DLNODE* pPrev;
			DLNODE* pNext;
		};
	protected:
		DLNODE* m_pDummy;

	public:
		CDLink() {
			m_pDummy = new DLNODE;
			m_pDummy->pPrev = m_pDummy->pNext = m_pDummy;
		}
		virtual ~CDLink() {
			if (m_pDummy != nullptr) {
				while (!Empty()) {
					DLNODE* pNode = Begin();
					Pop(pNode);
					delete pNode;
				}
				delete m_pDummy;
				m_pDummy = nullptr;
			}
		}

		DLNODE* Begin() { return m_pDummy->pNext; }
		DLNODE* End() { return m_pDummy; }


		void Push(DLNODE* pNode, DLNODE* pWhere) {
			DLNODE* pPrev = pWhere->pPrev;
			DLNODE* pNext = pWhere;
			pPrev->pNext = pNode;
			pNode->pPrev = pPrev;
			pNext->pPrev = pNode;
			pNode->pNext = pNext;
		}
		void Pop(DLNODE* pWhere) {
			DLNODE* pPrev = pWhere->pPrev;
			DLNODE* pNext = pWhere->pNext;
			pPrev->pNext = pNext;
			pNext->pPrev = pPrev;
		}

		void PushBack(DLNODE* pNode) { Push(pNode, End()); }
		void PopBack() { Pop(End()->pPrev); }
		void PushFront(DLNODE* pNode) { Push(pNode, Begin()); }
		void PopFront() { Pop(Begin()); }

		bool Empty() { return Begin() == End(); }
	};


	template<uint32_t _Size>
	class CFixedMemPool
	{
	public:
		enum :uint32_t {
			SEGMENT_BLOCK_COUNT = 256,
			BLOCK_FLAGS_ALLOCED = 1,
			MEM_DATA_SIZE = _Size
		};

		typedef uint8_t(MemData)[MEM_DATA_SIZE];
	protected:
		typedef CDLink::DLNODE DLNODE;

		struct NODE_BLOCK :DLNODE {
			uint32_t uiIndex;
			uint32_t uiFlags;
			MemData pData;
		};

		struct NODE_SEGMENT :DLNODE {
			uint32_t uiAllocCount;
			NODE_BLOCK pBlocks[SEGMENT_BLOCK_COUNT];
		};

		CDLink m_dlSegment;
		CDLink m_dlFreeBlock;
		uint32_t bbb[65540];

		uint32_t aaa[100];

		void AllocSegment() {
			NODE_SEGMENT* pSegment = new NODE_SEGMENT;
			pSegment->uiAllocCount = 0;
			m_dlSegment.PushBack(pSegment);
			for (uint32_t i = 0; i<SEGMENT_BLOCK_COUNT; i++) {
				NODE_BLOCK* pBlock = &(pSegment->pBlocks[i]);
				pBlock->uiIndex = i;
				pBlock->uiFlags = 0;
				m_dlFreeBlock.PushBack(pBlock);
			}
		}
		void FreeSegment(NODE_SEGMENT* pSegment) {
			assert(pSegment->uiAllocCount == 0);
			for (uint32_t i = 0; i<SEGMENT_BLOCK_COUNT; i++) {
				NODE_BLOCK* pBlock = &(pSegment->pBlocks[i]);
				m_dlFreeBlock.Pop(pBlock);
			}
			m_dlSegment.Pop(pSegment);
			delete[] pSegment;
		}

		NODE_SEGMENT* GetSegment(NODE_BLOCK* pBlock) {
			return (NODE_SEGMENT*)((uint8_t*)pBlock - offsetof(NODE_SEGMENT, pBlocks[pBlock->uiIndex]));
		}
		NODE_BLOCK* GetBlock(MemData& pData) {
			return (NODE_BLOCK*)((uint8_t*)pData - offsetof(NODE_BLOCK, pData));
		}
	public:
		MemData& Alloc() {
			if (m_dlFreeBlock.Empty())
				AllocSegment();

			NODE_BLOCK* pBlock = (NODE_BLOCK*)m_dlFreeBlock.Begin();
			m_dlFreeBlock.PopFront();
			assert(!(pBlock->uiFlags & BLOCK_FLAGS_ALLOCED));
			pBlock->uiFlags |= BLOCK_FLAGS_ALLOCED;

			NODE_SEGMENT* pSegment = GetSegment(pBlock);
			pSegment->uiAllocCount++;

			return pBlock->pData;
		}
		void Free(MemData& pData) {
			NODE_BLOCK* pBlock = GetBlock(pData);
			assert(pBlock->uiFlags & BLOCK_FLAGS_ALLOCED);
			pBlock->uiFlags &= ~BLOCK_FLAGS_ALLOCED;
			m_dlFreeBlock.PushBack(pBlock);

			NODE_SEGMENT* pSegment = GetSegment(pBlock);
			assert(pSegment->uiAllocCount>0);
			pSegment->uiAllocCount--;
		}

		void GarbageCollection() {
			for (DLNODE* pNode = m_dlSegment.Begin(); pNode != m_dlSegment.End();) {
				NODE_SEGMENT* pSegment = (NODE_SEGMENT*)pNode;
				pNode = pNode->pNext;
				if (pSegment->uiAllocCount == 0)
					FreeSegment(pSegment);
			}
		}

		CFixedMemPool() {}
		virtual ~CFixedMemPool() {
			GarbageCollection();
		}
	};
};



#endif
