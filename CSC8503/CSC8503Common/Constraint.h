#pragma once

namespace NCL {
	namespace CSC8503 {
		class Constraint	{
		public:
			Constraint() {}
			virtual ~Constraint() {}

			virtual void UpdateConstraint(float dt) = 0;
		};
	}
}
#pragma once
#include "Constraint.h"
#include "GameObject.h"
#include "../../Common/Vector3.h"

namespace NCL {
	namespace CSC8503 {
		class GameObject;
		
		// An implementation of a constraint
		class PositionConstraint : public Constraint {
			public:
				PositionConstraint(GameObject * a, GameObject * b, float d) {
					objectA = a;
					objectB = b;
					distance = d;
				}
				~PositionConstraint() {}
				
				void UpdateConstraint(float dt) override;
			protected:
				GameObject* objectA;
				GameObject* objectB;
				
				float distance;
		};
	}
}