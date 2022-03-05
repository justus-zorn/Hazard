// Copyright 2022 Justus Zorn

#include "Common.h"

using namespace Hazard;

void Input::Clear() {
	keyboardInputs.clear();
	buttonInputs.clear();
	mouseMotion = false;
	composition.clear();
	textInput = false;
}
