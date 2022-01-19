#include "Constraint.h"
#include "../../Common/Maths.h"
#include "../../Common/Window.h"

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

	// P Key to push pistons
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P) && pistonDirection == PistonDirection::Resting) {
		Vector3 impulse = moveConstraint * 400.0f;
		piston->GetPhysicsObject()->ApplyLinearImpulse(impulse);
		pistonDirection = PistonDirection::Contracting;
	}

	Vector3 limitConstraint;
	if (pistonDirection == PistonDirection::Contracting) {

		limitConstraint = Vector3(moveConstraint.x, moveConstraint.y, moveConstraint.z);
	}
	if (pistonDirection == PistonDirection::Retracting) {

		limitConstraint = Vector3(-moveConstraint.x, -moveConstraint.y, -moveConstraint.z);
	}
	
	Vector3 distanceTravelled = piston->GetTransform().GetPosition() - restingPosition;
	float d = abs(distanceTravelled.Length());
	
	if (d > 50 && pistonDirection == PistonDirection::Contracting) {
		pistonDirection = PistonDirection::Retracting;
	}
	else if (d < 5 && pistonDirection == PistonDirection::Retracting) {
		pistonDirection = PistonDirection::Resting;
		piston->GetTransform().SetPosition(restingPosition);
	}

	if (pistonDirection == PistonDirection::Contracting) {
		Vector3 currentPos = piston->GetTransform().GetPosition();
		piston->GetTransform().SetPosition(Vector3(currentPos.x, restingPosition.y, currentPos.z));
	}
	else if (pistonDirection == PistonDirection::Retracting) {
		piston->GetPhysicsObject()->ApplyLinearImpulse(-moveConstraint);
		Vector3 currentPos = piston->GetTransform().GetPosition();
		piston->GetTransform().SetPosition(Vector3(currentPos.x, restingPosition.y, currentPos.z));
	}
	else if (pistonDirection == PistonDirection::Resting) {
		piston->GetPhysicsObject()->SetLinearVelocity(Vector3(0.0f, 0.0f, 0.0f));
	}
}

void NCL::CSC8503::BalancingPlaneConstraint::UpdateConstraint(float dt) {

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		pushOrientation->x = 3.0f;
	}	
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		pushOrientation->x = -3.0f;
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		pushOrientation->z = -3.0f;
	}
	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		pushOrientation->z = 3.0f;
	}

	Vector3 relativePos = balancingPlane->GetTransform().GetPosition() - restingPosition;
	float currentDistance = relativePos.Length();
	//			MAX DISTANCE - currentDistance
	float offset = 30.0f - currentDistance;

	if (abs(offset) > 0.0f) {
		Vector3 offsetDir = relativePos.Normalised();

		PhysicsObject* phys = balancingPlane->GetPhysicsObject();
		Vector3 velocity = phys->GetLinearVelocity();
		float constraintMass = phys->GetInverseMass();

		if (constraintMass > 0.0f) {
			// how much of their relative force is affecting the constraint
			float velocityDot = Vector3::Dot(velocity, offsetDir);

			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * offset;

			float lambda = -(velocityDot + bias) / constraintMass;

			Vector3 impulse = offsetDir * lambda;

			phys->ApplyLinearImpulse((impulse)/500.0f);
			phys->ApplyAngularImpulse(*pushOrientation);
		}
	}

	*pushOrientation = Vector3(0.0f, 0.0f, 0.0f);
}