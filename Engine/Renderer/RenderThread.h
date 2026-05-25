#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <immintrin.h>
#include <thread>
#include <utility>

#include "../Profiling/Profiler.h"
#include "Vulkan/DrawData.h"

namespace gns::rendering
{
	class Renderer;
}

namespace gns
{
	enum class RenderSubmissionSlotState : uint32_t
	{
		Empty,
		Writing,
		Ready,
		Reading,
		Completed
	};

	struct RenderSubmissionSlot
	{
		std::atomic<RenderSubmissionSlotState> state = RenderSubmissionSlotState::Empty;
		RenderSubmission submission;
	};

	class RenderThread
	{
	public:
		RenderThread() = default;
		~RenderThread()
		{
			Stop();
		}

		template<typename ExecuteSubmission>
		void Start(rendering::Renderer& renderer, ExecuteSubmission&& executeSubmission)
		{
			GNS_PROFILE_FUNCTION();
			m_renderer = &renderer;
			m_executeSubmission = std::forward<ExecuteSubmission>(executeSubmission);
			m_running.store(true, std::memory_order_release);
			m_worker = std::thread([this]()
			{
				ThreadMain();
			});
		}

		void Stop()
		{
			GNS_PROFILE_FUNCTION();
			m_running.store(false, std::memory_order_release);
			//m_worker.detach();
			if (m_worker.joinable())
			{
				m_worker.join();
			}

			m_executeSubmission = {};
			m_renderer = nullptr;
		}

		bool IsRunning() const
		{
			return m_running.load(std::memory_order_acquire);
		}

		void Submit(RenderSubmission&& submission)
		{
			GNS_PROFILE_SCOPE("RenderThread::Submit");
			RenderSubmissionSlot& slot = AcquireEmptySlot();
			slot.submission = std::move(submission);
			slot.state.store(RenderSubmissionSlotState::Ready, std::memory_order_release);
		}

		void WaitForIdle()
		{
			GNS_PROFILE_SCOPE("RenderThread::WaitForIdle");
			while (HasBusySubmissions())
			{
				SpinPause();
			}
		}

		template<typename CompleteSubmission>
		void DrainCompletedSubmissions(CompleteSubmission&& completeSubmission)
		{
			GNS_PROFILE_SCOPE("RenderThread::DrainCompletedSubmissions");
			for (RenderSubmissionSlot& slot : m_slots)
			{
				RenderSubmissionSlotState expected = RenderSubmissionSlotState::Completed;
				if (!slot.state.compare_exchange_strong(
					expected,
					RenderSubmissionSlotState::Reading,
					std::memory_order_acq_rel,
					std::memory_order_acquire))
				{
					continue;
				}

				std::forward<CompleteSubmission>(completeSubmission)(slot.submission);
				slot.submission.Clear();
				slot.state.store(RenderSubmissionSlotState::Empty, std::memory_order_release);
			}
		}

	private:
		static constexpr size_t SlotCount = 2;

		static void SpinPause()
		{
			_mm_pause();
		}

		RenderSubmissionSlot& AcquireEmptySlot()
		{
			GNS_PROFILE_SCOPE("RenderThread::AcquireEmptySlot");
			size_t slotIndex = m_nextWriteSlot;
			while (true)
			{
				RenderSubmissionSlot& slot = m_slots[slotIndex];
				RenderSubmissionSlotState expected = RenderSubmissionSlotState::Empty;
				if (slot.state.compare_exchange_strong(
					expected,
					RenderSubmissionSlotState::Writing,
					std::memory_order_acq_rel,
					std::memory_order_acquire))
				{
					m_nextWriteSlot = (slotIndex + 1) % SlotCount;
					return slot;
				}

				slotIndex = (slotIndex + 1) % SlotCount;
				SpinPause();
			}
		}

		bool HasReadySubmissions() const
		{
			for (const RenderSubmissionSlot& slot : m_slots)
			{
				if (slot.state.load(std::memory_order_acquire) == RenderSubmissionSlotState::Ready)
				{
					return true;
				}
			}

			return false;
		}

		bool HasBusySubmissions() const
		{
			for (const RenderSubmissionSlot& slot : m_slots)
			{
				const RenderSubmissionSlotState state = slot.state.load(std::memory_order_acquire);
				if (state == RenderSubmissionSlotState::Writing ||
					state == RenderSubmissionSlotState::Ready ||
					state == RenderSubmissionSlotState::Reading)
				{
					return true;
				}
			}

			return false;
		}

		void ThreadMain()
		{
			GNS_PROFILE_THREAD("Genesis Render Thread");
			GNS_PROFILE_SCOPE("RenderThread::ThreadMain");
			while (m_running.load(std::memory_order_acquire) || HasReadySubmissions())
			{
				bool didWork = false;
				for (RenderSubmissionSlot& slot : m_slots)
				{
					RenderSubmissionSlotState expected = RenderSubmissionSlotState::Ready;
					if (!slot.state.compare_exchange_strong(
						expected,
						RenderSubmissionSlotState::Reading,
						std::memory_order_acq_rel,
						std::memory_order_acquire))
					{
						continue;
					}

					if (m_executeSubmission)
					{
						GNS_PROFILE_SCOPE("RenderThread::ExecuteSubmissionCallback");
						m_executeSubmission(slot.submission);
					}

					slot.state.store(RenderSubmissionSlotState::Completed, std::memory_order_release);
					didWork = true;
				}

				if (!didWork)
				{
					SpinPause();
				}
			}
		}

		rendering::Renderer* m_renderer = nullptr;
		std::function<void(RenderSubmission&)> m_executeSubmission;
		std::array<RenderSubmissionSlot, SlotCount> m_slots = {};
		std::thread m_worker;
		size_t m_nextWriteSlot = 0;
		std::atomic<bool> m_running = false;
	};
}
