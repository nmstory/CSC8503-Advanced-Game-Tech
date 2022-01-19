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

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		enum class PistonDirection {
			Contracting,
			Retracting,
			Resting
		};

		class PistonConstraint : public Constraint {
			public:
				PistonConstraint(GameObject* p, Vector3 moveConstraint) {
					this->piston = p;
					this->restingPosition = p->GetTransform().GetPosition();
					this->pushOrientation = new Vector3(0.0f, 0.0f, 0.0f);
					this->pistonDirection = PistonDirection::Resting;
					if (moveConstraint.x > 0) {
						allowMoveX = true;
					}
					else {
						allowMoveX = false;
					}
					
					if (moveConstraint.y > 0) {
						allowMoveY = true;
					}
					else {
						allowMoveY = false;
					}
					
					if (moveConstraint.z > 0) {
						allowMoveZ = true;
					}
					else {
						allowMoveZ = false;
					}
					this->moveConstraint = moveConstraint;
				}
				~PistonConstraint() {}
				
				void UpdateConstraint(float dt) override;
			protected:
				PistonDirection pistonDirection;
				Vector3 moveConstraint;
				GameObject* piston;
				Vector3 restingPosition;
				Vector3* pushOrientation;
				bool allowMoveX, allowMoveY, allowMoveZ;
		};
	}
}

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		class BalancingPlaneConstraint : public Constraint {
			public:
				BalancingPlaneConstraint(GameObject* p, Vector3 moveConstraint) {
					this->balancingPlane = p;
					this->restingPosition = p->GetTransform().GetPosition();
					this->pushOrientation = new Vector3(0.0f, 0.0f, 0.0f);
					if (moveConstraint.x > 0) {
						allowMoveX = true;
					}
					else {
						allowMoveX = false;
					}
					
					if (moveConstraint.y > 0) {
						allowMoveY = true;
					}
					else {
						allowMoveY = false;
					}
					
					if (moveConstraint.z > 0) {
						allowMoveZ = true;
					}
					else {
						allowMoveZ = false;
					}					
				}
				~BalancingPlaneConstraint() {}
				
				void UpdateConstraint(float dt) override;
			protected:
				GameObject* balancingPlane;
				Vector3 restingPosition;
				Vector3* pushOrientation;
				bool allowMoveX, allowMoveY, allowMoveZ;
		};
	}
}