#pragma once

#include <reactphysics3d/collision/OverlapCallback.h>

namespace Ember {

	class OverlapTestCallback : public reactphysics3d::OverlapCallback
	{
	public:
		virtual void onOverlap(reactphysics3d::OverlapCallback::CallbackData& callbackData) override
		{
			// If this function is called, our dummy body is touching something!
			// Note: callbackData contains an array of overlap pairs, so you could 
			// loop through them here to get the EntityIDs of what you hit!
			m_HasHit = true;
		}

		const bool HasHit() const { return m_HasHit; }

	private:
		bool m_HasHit = false;
	};
}