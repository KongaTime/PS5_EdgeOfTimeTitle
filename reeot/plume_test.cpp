/*
 * EdgeOfTimeRecomp for the PS5 - test app 5's picture: EdgeOfTimeRecomp's
 * renderer, plume, on the PS5 display.
 *
 * plume (EdgeOfTimeRecomp's thirdparty/plume, with its ps5/patches/
 * plume-ps5.patch) gets Vulkan from the RADV linked into the title and its
 * surface from VK_KHR_display. This creates its device, a command queue and a
 * swap chain on the display, then clears the screen to red, green, blue and
 * white for a second each, as EdgeOfTimeRecomp's renderer would present a
 * frame (the steps of plume's examples/triangle). plume's own messages
 * (stderr) go into the results file for the test's length.
 *
 * Copyright (C) 2026 KongaTime
 * SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "plume_render_interface.h"

extern "C" void reeot_note(const char *format, ...) __attribute__((format(printf, 1, 2)));

namespace plume
{
extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
}

namespace
{
using namespace plume;

const char *FormatName(RenderFormat format)
{
	switch (format) {
	case RenderFormat::B8G8R8A8_UNORM:
		return "B8G8R8A8_UNORM";
	case RenderFormat::R8G8B8A8_UNORM:
		return "R8G8B8A8_UNORM";
	default:
		return "another format";
	}
}

bool Run(const char *results)
{
	reeot_note("picture: plume's Vulkan interface (RADV, linked in)");
	std::unique_ptr<RenderInterface> renderInterface = CreateVulkanInterface();
	if (!renderInterface) {
		reeot_note("picture: FAILED to create plume's Vulkan interface");
		return false;
	}
	std::unique_ptr<RenderDevice> device = renderInterface->createDevice();
	if (!device) {
		reeot_note("picture: FAILED to create a device");
		return false;
	}
	reeot_note("picture: device created");
	std::unique_ptr<RenderCommandQueue> queue = device->createCommandQueue(RenderCommandListType::DIRECT);

	// The first format the display takes
	std::unique_ptr<RenderSwapChain> swapChain;
	for (RenderFormat format : {RenderFormat::B8G8R8A8_UNORM, RenderFormat::R8G8B8A8_UNORM}) {
		reeot_note("picture: a swap chain on the display in %s", FormatName(format));
		RenderWindow window = {0, 0};
		swapChain = queue->createSwapChain(RenderSwapChainDesc(window, format, 2));
		if (swapChain && swapChain->resize() && !swapChain->isEmpty())
			break;
		swapChain.reset();
	}
	if (!swapChain) {
		reeot_note("picture: FAILED: no swap chain on the display");
		return false;
	}
	reeot_note("picture: swap chain %ux%u, %u images", swapChain->getWidth(), swapChain->getHeight(),
		   swapChain->getTextureCount());

	std::vector<std::unique_ptr<RenderFramebuffer>> framebuffers;
	for (uint32_t i = 0; i < swapChain->getTextureCount(); i++) {
		const RenderTexture *color = swapChain->getTexture(i);
		RenderFramebufferDesc desc;
		desc.colorAttachments = &color;
		desc.colorAttachmentsCount = 1;
		desc.depthAttachment = nullptr;
		framebuffers.push_back(device->createFramebuffer(desc));
	}
	std::unique_ptr<RenderCommandList> list = queue->createCommandList();
	std::unique_ptr<RenderCommandSemaphore> acquired = device->createCommandSemaphore();
	std::unique_ptr<RenderCommandFence> fence = device->createCommandFence();
	std::vector<std::unique_ptr<RenderCommandSemaphore>> released;
	for (uint32_t i = 0; i < swapChain->getTextureCount(); i++)
		released.push_back(device->createCommandSemaphore());

	const RenderColor colors[] = {RenderColor(1.0f, 0.0f, 0.0f, 1.0f), RenderColor(0.0f, 1.0f, 0.0f, 1.0f),
				      RenderColor(0.0f, 0.0f, 1.0f, 1.0f), RenderColor(1.0f, 1.0f, 1.0f, 1.0f)};
	const char *names[] = {"red", "green", "blue", "white"};
	int frames = 0;
	for (int c = 0; c < 4; c++) {
		reeot_note("picture: the screen should be %s for a second now", names[c]);
		const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(1);
		while (std::chrono::steady_clock::now() < end) {
			uint32_t index = 0;
			if (!swapChain->acquireTexture(acquired.get(), &index)) {
				reeot_note("picture: FAILED to acquire a swap chain image (frame %d)", frames);
				return false;
			}
			RenderTexture *texture = swapChain->getTexture(index);
			list->begin();
			list->barriers(RenderBarrierStage::GRAPHICS,
				       RenderTextureBarrier(texture, RenderTextureLayout::COLOR_WRITE));
			list->setFramebuffer(framebuffers[index].get());
			list->clearColor(0, colors[c]);
			list->barriers(RenderBarrierStage::NONE, RenderTextureBarrier(texture, RenderTextureLayout::PRESENT));
			list->end();
			const RenderCommandList *lists = list.get();
			RenderCommandSemaphore *wait = acquired.get();
			RenderCommandSemaphore *signal = released[index].get();
			queue->executeCommandLists(&lists, 1, &wait, 1, &signal, 1, fence.get());
			if (!swapChain->present(index, &signal, 1)) {
				reeot_note("picture: FAILED to present (frame %d)", frames);
				return false;
			}
			queue->waitForCommandFence(fence.get());
			frames++;
		}
	}
	reeot_note("picture: %d frames presented in four seconds", frames);
	(void)results;
	return true;
}
} // namespace

extern "C" bool reeot_plume_test(const char *results)
{
	// plume reports its failures on stderr: into the results file meanwhile
	fflush(stderr);
	const int saved = dup(STDERR_FILENO);
	const int file = open(results, O_WRONLY | O_APPEND | O_CREAT, 0666);
	if (file >= 0) {
		dup2(file, STDERR_FILENO);
		close(file);
	}
	const bool passed = Run(results);
	fflush(stderr);
	if (saved >= 0) {
		dup2(saved, STDERR_FILENO);
		close(saved);
	}
	return passed;
}
