#include <sumire/core/rendering/sumi_gbuffer.hpp>

#include <sumire/util/vk_check_success.hpp>

#include <stdexcept>
#include <array>

namespace sumire {

    SumiGbuffer::SumiGbuffer(SumiDevice& device, uint32_t width, uint32_t height)
        : sumiDevice{ device }, width { width }, height { height }
    {
        init();
    }
    
    SumiGbuffer::~SumiGbuffer() {
        // Dispose of attachments
        destroyAttachment(position);
        destroyAttachment(albedo);
        destroyAttachment(normal);
        destroyAttachment(depth);

        // Destroy framebuffer
        vkDestroyFramebuffer(sumiDevice.device(), framebuffer, nullptr);

        // Destroy render pass
        vkDestroyRenderPass(sumiDevice.device(), renderPass, nullptr);

        // Destroy signaling semaphore
        vkDestroySemaphore(sumiDevice.device(), renderFinishedSemaphore, nullptr);
    }

    void SumiGbuffer::init() {
        createAttachments();
        createRenderPass();
        createFramebuffer();
        createSyncObjects();
    }

    void SumiGbuffer::createAttachments() {
        createAttachment(
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            position
        );

        createAttachment(
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            albedo
        );

        createAttachment(
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            normal
        );

        // Find supported depth format
        VkFormat depthFormat = sumiDevice.findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
        createAttachment(
            depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            depth
        );
    }

    void SumiGbuffer::createAttachment(
        VkFormat format, 
        VkImageUsageFlagBits usage,
        GbufferAttachment &attachment
    ) {
        attachment.format = format; // For setting up render pass & potentially querying from render pipeline

        // Allocate Memory & Create Image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT; // Allow Gbuffer Attachments to be used as descriptors
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0x0;

        sumiDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            attachment.image, 
            attachment.memory
        );

        VkImageAspectFlags aspectMask = 0x0;
        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) 
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        assert(aspectMask > 0 && "No suitable aspect mask provided");

        // Create Image View
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = attachment.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(sumiDevice.device(), &viewInfo, nullptr, &attachment.view) != VK_SUCCESS) {
            throw std::runtime_error("[Sumire::SumiGbuffer] Failed to create attachment image view");
        }
    }

    void SumiGbuffer::destroyAttachment(GbufferAttachment &attachment) {
        vkDestroyImageView(sumiDevice.device(), attachment.view, nullptr);
        vkDestroyImage(sumiDevice.device(), attachment.image, nullptr);
        vkFreeMemory(sumiDevice.device(), attachment.memory, nullptr);
    }

    void SumiGbuffer::createRenderPass() {
        // Base attachment descriptions with format left empty for later initialization
        VkAttachmentDescription baseColorAttachment{};
        baseColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        baseColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        baseColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        baseColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        baseColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        baseColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        baseColorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentDescription baseDepthAttachment{};
        baseDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        baseDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        baseDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        baseDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        baseDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        baseDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        baseDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::array<VkAttachmentDescription, 4> attachmentDescriptions{
            baseColorAttachment, // 0: position
            baseColorAttachment, // 1: albedo
            baseColorAttachment, // 2: normal
            baseDepthAttachment  // 3: depth
        };

        // Initialize specific attachment formats
        attachmentDescriptions[0].format = position.format;
        attachmentDescriptions[1].format = albedo.format;
        attachmentDescriptions[2].format = normal.format;
        attachmentDescriptions[3].format = depth.format;

        // Reference setup for RenderPassCreateInfo
        std::array<VkAttachmentReference, 3> colorAttachmentRefs{};
        colorAttachmentRefs[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        colorAttachmentRefs[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        colorAttachmentRefs[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // These dependencies tell *external render passes* when they can access the gbuffer data
        //   as we do not want subsequent render passes using incomplete gbuffer data.
        // It also specifies that the gbuffer should only start execution 
        //   once previous render passes are finished.
        std::array<VkSubpassDependency, 2> dependencies{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();
        
		if (vkCreateRenderPass(sumiDevice.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("[Sumire::SumiGbuffer] Failed to create g-buffer render pass.");
		}
    }

    void SumiGbuffer::createFramebuffer() {
        assert(renderPass != VK_NULL_HANDLE && "Cannot create framebuffer before renderpass is initialized");
        
        std::array<VkImageView, 4> attachments{};
        attachments[0] = position.view;
        attachments[1] = albedo.view;
        attachments[2] = normal.view;
        attachments[3] = depth.view;

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(
            sumiDevice.device(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) 
        {
            throw std::runtime_error("[Sumire::SumiGbuffer] Failed to create framebuffer.");
        }
    }

    void SumiGbuffer::createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_CHECK_SUCCESS(
            vkCreateSemaphore(sumiDevice.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphore),
            "[Sumire::SumiGbuffer] Failed to create render finish semaphore."
        );
    }

    void SumiGbuffer::beginRenderPass(VkCommandBuffer commandBuffer) {
        assert(renderPassStarted == false && "Tried to start a new render pass when another was already in progress.");

        std::array<VkClearValue, 4> attachmentClearValues{};
        attachmentClearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        attachmentClearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        attachmentClearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        attachmentClearValues[3].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { width, height };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(attachmentClearValues.size());
        renderPassInfo.pClearValues = attachmentClearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(height);
        viewport.width = static_cast<float>(width);
        viewport.height = -static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{ {0, 0}, {width, height}};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        renderPassStarted = true;
    }

    void SumiGbuffer::endRenderPass(VkCommandBuffer commandBuffer) {
        assert(renderPassStarted == true && "Tried to end a render pass when none was started.");
        vkCmdEndRenderPass(commandBuffer);
        renderPassStarted = false;
    }

    void SumiGbuffer::submitCommandBuffer(
        const VkCommandBuffer *commandBuffer,
        const uint32_t waitSemaphoreCount,
        const VkSemaphore *waitSemaphores,
        const VkPipelineStageFlags *waitDstStageMask
    ) {
        VkSubmitInfo submitInfo{};
        // (GPU->GPU) Synchronisation
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitDstStageMask;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphore;
        
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = commandBuffer;

        VK_CHECK_SUCCESS(
            vkQueueSubmit(sumiDevice.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE),
            "[Sumire::SumiGbuffer] Failed to submit command buffer."
        );
    }

}