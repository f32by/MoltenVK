/*
 * MVKCmdRenderPass.h
 *
 * Copyright (c) 2015-2021 The Brenwill Workshop Ltd. (http://www.brenwill.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "MVKCommand.h"
#include "MVKCommandBuffer.h"
#include "MVKDevice.h"
#include "MVKFramebuffer.h"
#include "MVKSmallVector.h"

#import <Metal/Metal.h>

class MVKRenderPass;
class MVKFramebuffer;


#pragma mark -
#pragma mark MVKCmdBeginRenderPassBase

/**
 * Abstract base class of MVKCmdBeginRenderPass.
 * Contains all pieces that are independent of the templated portions.
 */
class MVKCmdBeginRenderPassBase : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						const VkRenderPassBeginInfo* pRenderPassBegin,
						VkSubpassContents contents);

	inline MVKRenderPass* getRenderPass() { return _renderPass; }

protected:

	MVKRenderPass* _renderPass;
	MVKFramebuffer* _framebuffer;
	VkRect2D _renderArea;
	VkSubpassContents _contents;
};


#pragma mark -
#pragma mark MVKCmdBeginRenderPass

// Requires at least C++ 14.
constexpr bool isAny(size_t value, std::initializer_list<size_t> desired) {
	for(auto d: desired) {
		if(value == d) {
			return true;
		}
	}
	return false;
}

/**
 * Vulkan command to begin a render pass.
 * Template class to balance vector pre-allocations between very common low counts and fewer larger counts.
 */
template <size_t N_CV, size_t N_A,
		  bool COND_CV = isAny(N_CV, {1, 2, 9}),
		  bool COND_A = isAny(N_A, {0, 1, 2, 9})>
class MVKCmdBeginRenderPass : public MVKCmdBeginRenderPassBase {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						const VkRenderPassBeginInfo* pRenderPassBegin,
						VkSubpassContents contents);
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						const VkRenderPassBeginInfo* pRenderPassBegin,
						const VkSubpassBeginInfo* pSubpassBeginInfo);

	void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

	MVKSmallVector<VkClearValue, N_CV> _clearValues;
    MVKSmallVector<MVKImageView*, N_A> _attachments;
};

template <size_t N_CV, size_t N_A, bool COND_CV, bool COND_A>
VkResult MVKCmdBeginRenderPass<N_CV, N_A, COND_CV, COND_A>::setContent(MVKCommandBuffer* cmdBuff,
																	   const VkRenderPassBeginInfo* pRenderPassBegin,
																	   VkSubpassContents contents) {
	MVKCmdBeginRenderPassBase::setContent(cmdBuff, pRenderPassBegin, contents);

	// Add clear values
	uint32_t cvCnt = pRenderPassBegin->clearValueCount;
	_clearValues.clear();	// Clear for reuse
	_clearValues.reserve(cvCnt);
	for (uint32_t i = 0; i < cvCnt; i++) {
		_clearValues.push_back(pRenderPassBegin->pClearValues[i]);
	}

	bool imageless = false;
	for (auto* next = (const VkBaseInStructure*)pRenderPassBegin->pNext; next; next = next->pNext) {
		switch (next->sType) {
		case VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO: {
			const auto* pAttachmentBegin = (VkRenderPassAttachmentBeginInfo*)next;
			for(uint32_t i = 0; i < pAttachmentBegin->attachmentCount; i++) {
				_attachments.push_back((MVKImageView*)pAttachmentBegin->pAttachments[i]);
			}
			imageless = true;
			break;
		}
		default:
			break;
		}
	}
	
	if (!imageless) {
		for(uint32_t i = 0; i < _framebuffer->getAttachmentCount(); i++) {
			_attachments.push_back((MVKImageView*)_framebuffer->getAttachment(i));
		}
	}

	return VK_SUCCESS;
}

template <size_t N_CV, size_t N_A, bool COND_CV, bool COND_A>
VkResult MVKCmdBeginRenderPass<N_CV, N_A, COND_CV, COND_A>::setContent(MVKCommandBuffer* cmdBuff,
																	   const VkRenderPassBeginInfo* pRenderPassBegin,
																	   const VkSubpassBeginInfo* pSubpassBeginInfo) {
	return setContent(cmdBuff, pRenderPassBegin, pSubpassBeginInfo->contents);
}

template <size_t N_CV, size_t N_A, bool COND_CV, bool COND_A>
void MVKCmdBeginRenderPass<N_CV, N_A, COND_CV, COND_A>::encode(MVKCommandEncoder* cmdEncoder) {
//	MVKLogDebug("Encoding vkCmdBeginRenderPass(). Elapsed time: %.6f ms.", mvkGetElapsedMilliseconds());
	cmdEncoder->beginRenderpass(this,
								_contents,
								_renderPass,
								_framebuffer->getExtent2D(),
								_framebuffer->getLayerCount(),
								_renderArea,
								_clearValues.contents(),
								_attachments.contents());
}

// Concrete template class implementations.
typedef MVKCmdBeginRenderPass<1, 0, true, true> MVKCmdBeginRenderPass10;
typedef MVKCmdBeginRenderPass<2, 0, true, true> MVKCmdBeginRenderPass20;
typedef MVKCmdBeginRenderPass<9, 0, true, true> MVKCmdBeginRenderPassMulti0;

typedef MVKCmdBeginRenderPass<1, 1, true, true> MVKCmdBeginRenderPass11;
typedef MVKCmdBeginRenderPass<2, 1, true, true> MVKCmdBeginRenderPass21;
typedef MVKCmdBeginRenderPass<9, 1, true, true> MVKCmdBeginRenderPassMulti1;

typedef MVKCmdBeginRenderPass<1, 2, true, true> MVKCmdBeginRenderPass12;
typedef MVKCmdBeginRenderPass<2, 2, true, true> MVKCmdBeginRenderPass22;
typedef MVKCmdBeginRenderPass<9, 2, true, true> MVKCmdBeginRenderPassMulti2;

typedef MVKCmdBeginRenderPass<1, 9, true, true> MVKCmdBeginRenderPass1Multi;
typedef MVKCmdBeginRenderPass<2, 9, true, true> MVKCmdBeginRenderPass2Multi;
typedef MVKCmdBeginRenderPass<9, 9, true, true> MVKCmdBeginRenderPassMultiMulti;


#pragma mark -
#pragma mark MVKCmdNextSubpass

/** Vulkan command to begin a render pass. */
class MVKCmdNextSubpass : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						VkSubpassContents contents);
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						const VkSubpassBeginInfo* pSubpassBeginInfo,
						const VkSubpassEndInfo* pSubpassEndInfo);

	void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

	VkSubpassContents _contents;
};


#pragma mark -
#pragma mark MVKCmdEndRenderPass

/** Vulkan command to end the current render pass. */
class MVKCmdEndRenderPass : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff);
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						const VkSubpassEndInfo* pSubpassEndInfo);

	void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

};


#pragma mark -
#pragma mark MVKCmdExecuteCommands

/**
 * Vulkan command to execute secondary command buffers.
 * Template class to balance vector pre-allocations between very common low counts and fewer larger counts.
 */
template <size_t N>
class MVKCmdExecuteCommands : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						uint32_t commandBuffersCount,
						const VkCommandBuffer* pCommandBuffers);

	void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

	MVKSmallVector<MVKCommandBuffer*, N> _secondaryCommandBuffers;
};

// Concrete template class implementations.
typedef MVKCmdExecuteCommands<1> MVKCmdExecuteCommands1;
typedef MVKCmdExecuteCommands<16> MVKCmdExecuteCommandsMulti;


#pragma mark -
#pragma mark MVKCmdSetViewport

/**
 * Vulkan command to set the viewports.
 * Template class to balance vector pre-allocations between very common low counts and fewer larger counts.
 */
template <size_t N>
class MVKCmdSetViewport : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						uint32_t firstViewport,
						uint32_t viewportCount,
						const VkViewport* pViewports);

	void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

	MVKSmallVector<VkViewport, N> _viewports;
	uint32_t _firstViewport;
};

// Concrete template class implementations.
typedef MVKCmdSetViewport<1> MVKCmdSetViewport1;
typedef MVKCmdSetViewport<kMVKCachedViewportScissorCount> MVKCmdSetViewportMulti;


#pragma mark -
#pragma mark MVKCmdSetScissor

/**
 * Vulkan command to set the scissor rectangles.
 * Template class to balance vector pre-allocations between very common low counts and fewer larger counts.
 */
template <size_t N>
class MVKCmdSetScissor : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						uint32_t firstScissor,
						uint32_t scissorCount,
						const VkRect2D* pScissors);

	void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

	MVKSmallVector<VkRect2D, N> _scissors;
	uint32_t _firstScissor;
};

// Concrete template class implementations.
typedef MVKCmdSetScissor<1> MVKCmdSetScissor1;
typedef MVKCmdSetScissor<kMVKCachedViewportScissorCount> MVKCmdSetScissorMulti;


#pragma mark -
#pragma mark MVKCmdSetLineWidth

/** Vulkan command to set the line width. */
class MVKCmdSetLineWidth : public MVKCommand {

public:
    VkResult setContent(MVKCommandBuffer* cmdBuff,
					float lineWidth);

    void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

    float _lineWidth;
};


#pragma mark -
#pragma mark MVKCmdSetDepthBias

/** Vulkan command to set the depth bias. */
class MVKCmdSetDepthBias : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						float depthBiasConstantFactor,
						float depthBiasClamp,
						float depthBiasSlopeFactor);

    void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

    float _depthBiasConstantFactor;
    float _depthBiasClamp;
    float _depthBiasSlopeFactor;
};


#pragma mark -
#pragma mark MVKCmdSetBlendConstants

/** Vulkan command to set the blend constants. */
class MVKCmdSetBlendConstants : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						const float blendConst[4]);

    void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

    float _red;
    float _green;
    float _blue;
    float _alpha;
};


#pragma mark -
#pragma mark MVKCmdSetDepthBounds

/** Vulkan command to set depth bounds. */
class MVKCmdSetDepthBounds : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						float minDepthBounds,
						float maxDepthBounds);

    void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

    float _minDepthBounds;
    float _maxDepthBounds;
};


#pragma mark -
#pragma mark MVKCmdSetStencilCompareMask

/** Vulkan command to set the stencil compare mask. */
class MVKCmdSetStencilCompareMask : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						VkStencilFaceFlags faceMask,
						uint32_t stencilCompareMask);

    void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

    VkStencilFaceFlags _faceMask;
    uint32_t _stencilCompareMask;
};


#pragma mark -
#pragma mark MVKCmdSetStencilWriteMask

/** Vulkan command to set the stencil write mask. */
class MVKCmdSetStencilWriteMask : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						VkStencilFaceFlags faceMask,
						uint32_t stencilWriteMask);

    void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

    VkStencilFaceFlags _faceMask;
    uint32_t _stencilWriteMask;
};


#pragma mark -
#pragma mark MVKCmdSetStencilReference

/** Vulkan command to set the stencil reference value. */
class MVKCmdSetStencilReference : public MVKCommand {

public:
	VkResult setContent(MVKCommandBuffer* cmdBuff,
						VkStencilFaceFlags faceMask,
						uint32_t stencilReference);

    void encode(MVKCommandEncoder* cmdEncoder) override;

protected:
	MVKCommandTypePool<MVKCommand>* getTypePool(MVKCommandPool* cmdPool) override;

    VkStencilFaceFlags _faceMask;
    uint32_t _stencilReference;
};

