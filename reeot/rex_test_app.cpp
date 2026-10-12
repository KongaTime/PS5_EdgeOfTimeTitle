/*
 * EdgeOfTimeRecomp for the PS5 - test app 3: the ReXGlue runtime on the
 * console, through the PS5 app layer the fork ported from
 * TheSimpsonsGame-PS5-Builder (rexglue-sdk's ps5/PORTED.md).
 *
 * The title's main calls rex_ps5_app_main (reeot/title_main.cpp), which runs
 * this app on the ported app context and window. It checks, in order:
 *
 * 1. the window: the display surface's size;
 * 2. the guest memory: Memory::Initialize, as the game will call it, which
 *    also installs the MMIO handler and so makes the title's code readable
 *    (the fork's ExceptionHandler::Install); a read of this app's code then;
 * 3. the sound: a 440 Hz tone for two seconds through the ported sceAudioOut
 *    driver, fed from guest memory as the guest's audio client feeds it;
 * 4. the controller: a short rumble, then for 20 seconds each change of the
 *    buttons and triggers and each keystroke (CreateDefaultInputSystem, the
 *    ported scePad driver);
 *
 * then quits through the app context. Every line goes to the results file
 * before its step (reeot_note), so a file that stops says where.
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
#include <rex/input/input.h>
#include <rex/input/input_system.h>
#include <rex/system/xmemory.h>
#include <rex/thread.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context.h>

extern "C" void reeot_note(const char *format, ...) __attribute__((format(printf, 1, 2)));

namespace
{
using Clock = std::chrono::steady_clock;

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
		reeot_note("==== test app 3: the ReXGlue runtime ====");
		reeot_note("window: creating it");
		window_ = rex::ui::Window::Create(app_context(), "EdgeOfTimeRecomp", 1920, 1080);
		if (!window_ || !window_->Open()) {
			reeot_note("window: FAILED to create or open");
			return false;
		}
		reeot_note("window: open, %ux%u", window_->GetActualPhysicalWidth(),
			   window_->GetActualPhysicalHeight());

		reeot_note("memory: Memory::Initialize (the 4.5 GiB guest arena, the MMIO handler)");
		memory_ = std::make_unique<rex::memory::Memory>();
		if (!memory_->Initialize()) {
			reeot_note("memory: FAILED to initialize");
			return false;
		}
		reeot_note("memory: initialized");
		reeot_note("code: reading 16 bytes of this app's code (if this is the file's last line, "
			   "the code is still execute-only)");
		const volatile uint8_t *code = reinterpret_cast<const volatile uint8_t *>(&RexTestApp::Create);
		uint8_t bytes[16];
		for (int i = 0; i < 16; i++)
			bytes[i] = code[i];
		reeot_note("code: readable: %02x %02x %02x %02x %02x %02x %02x %02x ...", bytes[0], bytes[1],
			   bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);

		worker_ = std::thread([this] { Run(); });
		return true;
	}

	void OnDestroy() override
	{
		stopping_ = true;
		if (worker_.joinable())
			worker_.join();
		input_.reset();
		window_.reset();
		memory_.reset();
		reeot_note("==== test app 3: done ====");
	}

    private:
	explicit RexTestApp(rex::ui::WindowedAppContext &context) : WindowedApp(context, "reeot_test") {}

	void Run()
	{
		TestSound();
		TestController();
		reeot_note("quitting through the app context");
		app_context().CallInUIThread([this] { app_context().QuitFromUIThread(); });
	}

	void TestSound()
	{
		using rex::audio::prospero::kSamplesPerFrame;
		constexpr size_t kGuestChannels = 6;
		constexpr int kFrames = 375; // two seconds of 256-sample periods at 48 kHz
		reeot_note("sound: opening the PS5 driver (sceAudioOut)");
		auto semaphore = rex::thread::Semaphore::Create(64, 64);
		auto driver = std::make_unique<rex::audio::prospero::ProsperoAudioDriver>(memory_.get(),
											   semaphore.get());
		if (!driver->Initialize()) {
			reeot_note("sound: FAILED to open the driver");
			return;
		}
		const uint32_t frame = memory_->SystemHeapAlloc(uint32_t(kSamplesPerFrame * kGuestChannels * 4));
		if (!frame) {
			reeot_note("sound: FAILED to allocate a guest frame");
			return;
		}
		auto *planes = memory_->TranslateVirtual<uint32_t *>(frame);
		reeot_note("sound: a 440 Hz tone for two seconds (you should hear it now)");
		const auto start = Clock::now();
		double phase = 0;
		int submitted = 0;
		for (; submitted < kFrames && !stopping_; submitted++) {
			if (rex::thread::Wait(semaphore.get(), false, std::chrono::milliseconds(1000)) !=
			    rex::thread::WaitResult::kSuccess) {
				reeot_note("sound: no frame was played for a second; stopping");
				break;
			}
			// Guest planes: FL, FR, FC, LFE, BL, BR, big-endian floats
			for (size_t i = 0; i < kSamplesPerFrame; i++) {
				const float value = float(0.25 * std::sin(phase));
				phase += 2.0 * 3.14159265358979 * 440.0 / 48000.0;
				for (size_t channel = 0; channel < kGuestChannels; channel++)
					planes[channel * kSamplesPerFrame + i] =
						BigEndianFloat(channel < 2 ? value : 0.0f);
			}
			driver->SubmitFrame(frame);
		}
		const auto elapsed =
			std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
		reeot_note("sound: %d frames submitted in %lld ms (played at 5.33 ms a frame, after 64 queued)",
			   submitted, static_cast<long long>(elapsed));
		driver->Shutdown();
		memory_->SystemHeapFree(frame);
		reeot_note("sound: driver closed");
	}

	void TestController()
	{
		reeot_note("controller: CreateDefaultInputSystem (the PS5 scePad driver)");
		input_ = rex::input::CreateDefaultInputSystem(false);
		if (!input_) {
			reeot_note("controller: FAILED to create the input system");
			return;
		}
		rex::input::X_INPUT_CAPABILITIES caps{};
		const rex::X_RESULT caps_result = input_->GetCapabilities(0, rex::input::X_INPUT_FLAG_GAMEPAD, &caps);
		reeot_note("controller: capabilities for player 1: 0x%08x, type %u, flags 0x%x",
			   static_cast<unsigned>(caps_result), unsigned(caps.type), unsigned(caps.flags));

		rex::input::X_INPUT_VIBRATION vibration{};
		vibration.left_motor_speed = 0xffff;
		vibration.right_motor_speed = 0xffff;
		const rex::X_RESULT rumble = input_->SetState(0, &vibration);
		reeot_note("controller: rumble on: 0x%08x (you should feel it for half a second)",
			   static_cast<unsigned>(rumble));
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		vibration.left_motor_speed = 0;
		vibration.right_motor_speed = 0;
		input_->SetState(0, &vibration);

		reeot_note("controller: press buttons, the triggers and the sticks for 20 seconds");
		const auto end = Clock::now() + std::chrono::seconds(20);
		rex::X_RESULT last_result = 0xffffffff;
		uint32_t last_field = 0xffffffff;
		int lines = 0;
		while (Clock::now() < end && !stopping_ && lines < 400) {
			rex::input::X_INPUT_STATE state{};
			const rex::X_RESULT result = input_->GetState(0, &state);
			if (result != last_result) {
				reeot_note("controller: player 1 GetState 0x%08x%s", static_cast<unsigned>(result),
					   result == 0 ? " (connected)" : "");
				last_result = result;
				lines++;
			}
			if (result == 0) {
				const auto &pad = state.gamepad;
				const auto axis = [](int16_t value) { return value > 16000 ? 1u : value < -16000 ? 2u : 0u; };
				const uint32_t field = uint32_t(uint16_t(pad.buttons)) |
						       (uint32_t(uint8_t(pad.left_trigger) > 64) << 16) |
						       (uint32_t(uint8_t(pad.right_trigger) > 64) << 17) |
						       (axis(int16_t(pad.thumb_lx)) << 18) | (axis(int16_t(pad.thumb_ly)) << 20) |
						       (axis(int16_t(pad.thumb_rx)) << 22) | (axis(int16_t(pad.thumb_ry)) << 24);
				if (field != last_field) {
					reeot_note("controller: buttons 0x%04x, L2 %u, R2 %u, sticks L %d,%d R %d,%d (packet %u)",
						   unsigned(uint16_t(pad.buttons)), unsigned(uint8_t(pad.left_trigger)),
						   unsigned(uint8_t(pad.right_trigger)), int(int16_t(pad.thumb_lx)),
						   int(int16_t(pad.thumb_ly)), int(int16_t(pad.thumb_rx)),
						   int(int16_t(pad.thumb_ry)), unsigned(uint32_t(state.packet_number)));
					last_field = field;
					lines++;
				}
			}
			rex::input::X_INPUT_KEYSTROKE keystroke{};
			while (input_->GetKeystroke(0, 0, &keystroke) == 0 && lines < 400) {
				reeot_note("controller: keystroke vk 0x%04x flags 0x%04x",
					   unsigned(uint16_t(keystroke.virtual_key)), unsigned(uint16_t(keystroke.flags)));
				lines++;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
		reeot_note("controller: done");
	}

	std::unique_ptr<rex::ui::Window> window_;
	std::unique_ptr<rex::memory::Memory> memory_;
	std::unique_ptr<rex::input::InputSystem> input_;
	std::thread worker_;
	std::atomic<bool> stopping_{false};
};
} // namespace

REX_DEFINE_APP(reeot_test, RexTestApp::Create);
