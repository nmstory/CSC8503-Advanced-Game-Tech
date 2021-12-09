#include "Constraint.h"

void NCL::CSC8503::PositionConstraint::UpdateConstraint(float dt) {
	Vector3 relativePos = objectA->GetTransform().GetPosition() - objectB->GetTransform().GetPosition();
	float currentDistance = relativePos.Length();
	float offset = distance - currentDistance;

	// Are we breaking the constraint?
	if (abs(offset) > 0.0f) {
		Vector3 offsetDir = relativePos.Normalised();

		PhysicsObject* physA = objectA->GetPhysicsObject();
		PhysicsObject* physB = objectB->GetPhysicsObject();

		Vector3 relativeVelocity = physA->GetLinearVelocity() - physB->GetLinearVelocity();

		float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();

		if (constraintMass > 0.0f) {
			// how much of their relative force is affecting the constraint
			float velocityDot = Vector3::Dot(relativeVelocity, offsetDir);

			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * offset;

			float lambda = -(velocityDot + bias) / constraintMass;

			Vector3 aImpulse = offsetDir * lambda;
			Vector3 bImpulse = -offsetDir * lambda;

			physA->ApplyLinearImpulse(aImpulse); // multiplied by mass here
			physB->ApplyLinearImpulse(bImpulse); // multiplied by mass here
		}
	}
}

void NCL::CSC8503::PistonConstraint::UpdateConstraint(float dt) {

	Vector3 relativePos = piston->GetTransform().GetPosition() - restingPosition;
	//Vector3 relativePos = restingPosition- piston->GetTransform().GetPosition();
	float currentDistance = relativePos.Length();
	//			MAX DISTANCE - currentDistance
	float offset = 30 - currentDistance;

	if (abs(offset) > 0.0f) {
		Vector3 offsetDir = relativePos.Normalised();

		PhysicsObject* phys = piston->GetPhysicsObject();
		Vector3 velocity = phys->GetLinearVelocity();
		float constraintMass = phys->GetInverseMass();

		if (constraintMass > 0.0f) {
			// how much of their relative force is affecting the constraint
			float velocityDot = Vector3::Dot(velocity, offsetDir);

			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * offset;

			float lambda = -(velocityDot + bias) / constraintMass;

			Vector3 impulse = offsetDir * lambda;

			phys->ApplyLinearImpulse((impulse)/500);
		}
	}






	/*
	Vector3 pistonPosition = piston->GetTransform().GetPosition();

	if (allowMoveY && pistonPosition.y < restingPosition.y) {
		piston->GetTransform().SetPosition(Vector3(pistonPosition.x, restingPoint.y, pistonPosition.z));
	}
	*/
}