#pragma once


#pragma once
#include <functional>

namespace NCL {
	namespace CSC8503 {
		typedef std::function <void(float)> StateUpdateFunction;
		
		class State {
			public:
				State() {}
				State(StateUpdateFunction someFunc) {
					func = someFunc;
				}
				void Update(float dt) {
					if (func != nullptr) {
						func(dt);
					}
				}
			protected:
				StateUpdateFunction func;
		};
	}
}

/*

namespace NCL {
	namespace CSC8503 {
		class State		{
		public:
			typedef void(*StateFunc)(void*);

			State() {}
			State(StateFunc someFunc, void* someData) {
				func = someFunc;
				funcData = someData;
			}
			~State() {}
			void Update() {
				if (funcData != nullptr) {
					func(funcData);
				}
			}
		protected:
			StateFunc func;
			void* funcData;
		};


		
		

		class GenericState : public State		{
		public:
			GenericState(StateFunc someFunc, void* someData) {
				func		= someFunc;
				funcData	= someData;
			}
			virtual void Update() {
				if (funcData != nullptr) {
					func(funcData);
				}
			}
		
		
	}
}*/