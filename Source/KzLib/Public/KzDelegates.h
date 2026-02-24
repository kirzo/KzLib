// Copyright 2026 kirzo

#pragma once

#include "Delegates/Delegate.h"
#include "KzDelegates.generated.h"

class AActor;

/** Generic parameterless dynamic multicast delegate for simple events (e.g., OnFinished, OnDirectionChanged). */
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKzSimpleEventSignature);

/** Generic dynamic multicast delegate passing an Actor pointer (e.g., OnTargetReached, OnActorEntered). */
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzActorEventSignature, AActor*, Actor);

/** Generic dynamic multicast delegate passing a boolean (e.g., OnStateToggled). */
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKzBoolEventSignature, bool, bValue);