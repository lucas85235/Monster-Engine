#pragma once

// AI System - Main include file
// Include this single header to access all AI functionality

// Core
#include "engine/ai/Blackboard.h"
#include "engine/ai/StateTreeNode.h"
#include "engine/ai/StateNode.h"
#include "engine/ai/StateTree.h"
#include "engine/ai/Conditions.h"

// Built-in States
#include "engine/ai/states/IdleState.h"
#include "engine/ai/states/PatrolState.h"
#include "engine/ai/states/ChaseState.h"
#include "engine/ai/states/FleeState.h"
#include "engine/ai/states/MoveToState.h"
