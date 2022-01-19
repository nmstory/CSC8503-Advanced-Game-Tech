#pragma once
#include "Transform.h"
#include "CollisionVolume.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include <vector>

using std::vector;

namespace NCL {
	namespace CSC8503 {
		class GameObject	{
		public:
			GameObject(string name = "");
			~GameObject();

			void SetBoundingVolume(CollisionVolume* vol) {
				boundingVolume = vol;
			}

			const CollisionVolume* GetBoundingVolume() const {
				return boundingVolume;
			}

			bool IsActive() const {
				return isActive;
			}

			void SetIsActive(bool b) {
				isActive = b;
			}

			Transform& GetTransform() {
				return transform;
			}

			RenderObject* GetRenderObject() const {
				return renderObject;
			}

			PhysicsObject* GetPhysicsObject() const {
				return physicsObject;
			}

			void SetRenderObject(RenderObject* newObject) {
				renderObject = newObject;
			}

			void SetPhysicsObject(PhysicsObject* newObject) {
				physicsObject = newObject;
			}

			const string& GetName() const {
				return name;
			}
			
			void SetName(const std::string& n) {
				this->name = n;
			}

			virtual void OnCollisionBegin(GameObject* otherObject) {
				if (otherObject->GetName() == "coin") {
					score += 100;
					//otherObject->isActive = false;
					otherObject->setToDelete(true);
					otherObject->SetIsActive(false);
					//delete otherObject;
				}
				
			}

			virtual void OnCollisionEnd(GameObject* otherObject) {
			}

			bool GetBroadphaseAABB(Vector3&outsize) const;

			void UpdateBroadphaseAABB();

			void SetWorldID(int newID) {
				worldID = newID;
			}

			int	GetWorldID() const {
				return worldID;
			}

			int GetScore() { return score; }

			bool isToDelete() { return toDelete; }
			void setToDelete(bool b) { this->toDelete = b; }

		protected:
			Transform			transform;

			CollisionVolume*	boundingVolume;
			PhysicsObject*		physicsObject;
			RenderObject*		renderObject;

			bool	isActive;
			int		worldID;
			string	name;

			Vector3 broadphaseAABB;
			int		score;
			bool toDelete = false;
		};
	}
}

