/*
 * EdgeOfTimeRecomp for the PS5 - test app 5: EdgeOfTimeRecomp's renderer,
 * plume, on the PS5 display, inside the ReXGlue runtime.
 *
 * The title's main calls rex_ps5_app_main (reeot/title_main.cpp), which runs
 * this app on the PS5 app layer the fork ported from TheSimpsonsGame-PS5-
 * Builder (rexglue-sdk's ps5/PORTED.md). It checks, in order:
 *
 * 1. the window: the display surface's size;
 * 2. the picture: plume's device and a swap chain on the display, cleared to
 *    red, green, blue and white for a second each (reeot/plume_test.cpp);
 * 3. the guest memory: Memory::Initialize, and a read of this app's code;
 * 4. the sound: a one-second 440 Hz tone through the ported sceAudioOut driver;
 *
 * then quits through the app context. Every line goes to the results file
 * before its step (reeot_note), so a file that stops says where.
 *
 * Test apps 3 and 4 (2026-10-12, reeot/results) checked the window, memory,
 * sound, every button and stick of the controller and the fault records; their
 * controller and crash steps are in this file's history (a6dad73).
 *
 * Copyright (C) 2026 KongaTime
 * SPDX-License-Identifier: MIT
 */

#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <thread>

#include <rex/audio/prospero/prospero_audio_driver.h>
#include <rex/system/xmemory.h>
#include <rex/thread.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context.h>

extern "C" void reeot_note(const char *format, ...) __attribute__((format(printf, 1, 2)));
extern "C" bool reeot_plume_test(const char *results);

namespace
{
uint32_t BigEndianFloat(float value)
{
	return __builtin_bswap32(std::bit_cast<uint32_t>(value));
}

class RexTestApp final : public rex::ui::WindowedApp {
    public:
	static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext &context)
	{
		return std::unique_ptr<rex::ui::WindowedApp>(new RexTestApp(context));
	}

	bool OnInitialize() override
	{
		reeot_note("==== test app 5: EdgeOfTimeRecomp's renderer (plume) on the PS5 display ====");
		reeot_note("window: creating it");
		window_ = rex::ui::Window::Create(app_context(), "EdgeOfTimeRecomp");
		if (!window_ || !window_->Open()) {
			reeot_note("window: FAILED to create or open");
			return false;
		}
		reeot_note("window: open, %ux%u", window_->GetActualPhysicalWidth(),
			   window_->GetActualPhysicalHeight());
		worker_ = std::thread([this] { Run(); });
		return true;
	}

	void OnDestroy() override
	{
		stopping_ = true;
		if (worker_.joinable())
			worker_.join();
		window_.reset();
		memory_.reset();
		reeot_note("==== test app 5: done ====");
	}

    private:
	explicit RexTestApp(rex::ui::WindowedAppContext &context) : WindowedApp(context, "reeot_test") {}

	void Run()
	{
		reeot_note("picture: %s", reeot_plume_test(REEOT_TEST_RESULTS) ? "passed" : "FAILED");
		if (TestMemory())
			TestSound();
		reeot_note("quitting through the app context");
		app_context().CallInUIThread([this] { app_context().QuitFromUIThread(); });
	}

	bool TestMemory()
	{
		reeot_note("memory: Memory::Initialize (the 4.5 GiB guest arena, the MMIO handler)");
		memory_ = std::make_unique<rex::memory::Memory>();
		if (!memory_->Initialize()) {
			reeot_note("memory: FAILED to initialize");
			return false;
		}
		reeot_note("memory: initialized");
		const volatile uint8_t *code = reinterpret_cast<const volatile uint8_t *>(&RexTestApp::Create);
		uint8_t bytes[4];
		for (int i = 0; i < 4; i++)
			bytes[i] = code[i];
		reeot_note("code: readable: %02x %02x %02x %02x", bytes[0], bytes[1], bytes[2], bytes[3]);
		return true;
	}

	void TestSound()
	{
		using rex::audio::prospero::kSamplesPerFrame;
		constexpr size_t kGuestChannels = 6;
		constexpr int kFrames = 188; // a second of 256-sample periods at 48 kHz
		reeot_note("sound: a one-second beep through the PS5 driver (sceAudioOut)");
		auto semaphore = rex::thread::Semaphore::Create(64, 64);
		auto driver = std::make_unique<rex::audio::prospero::ProsperoAudioDriver>(memory_.get(),
											   semaphore.get());
		if (!driver->Initialize()) {
			reeot_note("sound: FAILED to open the driver");
			return;
		}
		const uint32_t frame = memory_->SystemHeapAlloc(uint32_t(kSamplesPerFrame * kGuestChannels * 4));
		auto *planes = memory_->TranslateVirtual<uint32_t *>(frame);
		double phase = 0;
		int submitted = 0;
		for (; submitted < kFrames && !stopping_; submitted++) {
			if (rex::thread::Wait(semaphore.get(), false, std::chrono::milliseconds(1000)) !=
			    rex::thread::WaitResult::kSuccess)
				break;
			for (size_t i = 0; i < kSamplesPerFrame; i++) {
				const float value = float(0.25 * std::sin(phase));
				phase += 2.0 * 3.14159265358979 * 440.0 / 48000.0;
				for (size_t channel = 0; channel < kGuestChannels; channel++)
					planes[channel * kSamplesPerFrame + i] =
						BigEndianFloat(channel < 2 ? value : 0.0f);
			}
			driver->SubmitFrame(frame);
		}
		driver->Shutdown();
		memory_->SystemHeapFree(frame);
		reeot_note("sound: %d frames played", submitted);
	}

	std::unique_ptr<rex::ui::Window> window_;
	std::unique_ptr<rex::memory::Memory> memory_;
	std::thread worker_;
	std::atomic<bool> stopping_{false};
};
} // namespace

REX_DEFINE_APP(reeot_test, RexTestApp::Create);
