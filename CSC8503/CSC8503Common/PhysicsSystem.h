#pragma once
#include "../CSC8503Common/GameWorld.h"
#include "QuadTree.h"
#include "../../Common/Vector2.h"
#include <set>

namespace NCL {
	namespace CSC8503 {
		class PhysicsSystem	{
		public:
			PhysicsSystem(GameWorld& g);
			~PhysicsSystem();

			void Clear();

			void Update(float dt);

			void UseGravity(bool state) {
				applyGravity = state;
			}

			void SetGlobalDamping(float d) {
				globalDamping = d;
			}

			void SetGravity(const Vector3& g);

			void SetLinearDamping(float d) {
				linearDamping = d;
			}
			float GetLinearDamping() {
				return linearDamping;
			}

			std::set<CollisionDetection::CollisionInfo>* PotentialCollisionsFromRay(Ray& r);

		protected:
			void BasicCollisionDetection(float dt);
			void BroadPhase();
			void NarrowPhase();

			void ClearForces();

			void IntegrateAccel(float dt);
			void IntegrateVelocity(float dt);

			void UpdateConstraints(float dt);

			void UpdateCollisionList();
			void UpdateObjectAABBs();

			void ImpulseResolveCollision(GameObject& a, GameObject&b, CollisionDetection::ContactPoint& p) const;
			void ResolveSpringCollision(GameObject& a, GameObject&b, CollisionDetection::ContactPoint& p, float dt) const;

			GameWorld& gameWorld;

			bool	applyGravity;
			Vector3 gravity;
			float	dTOffset;
			float	globalDamping;
			float	linearDamping = 0.4f; // Tutorial 2 Q.2.

			std::set<CollisionDetection::CollisionInfo> allCollisions; // used??
			std::set<CollisionDetection::CollisionInfo> broadphaseCollisions;

			// new
			std::set<CollisionDetection::CollisionInfo> dynamicCollisionObject;
			std::set<CollisionDetection::CollisionInfo> staticCollisionObject;


			NCL::CSC8503::QuadTree<GameObject*>* tree;

			bool useBroadPhase		= true;
			int numCollisionFrames	= 5;
		};
	}
}

