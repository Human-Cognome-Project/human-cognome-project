#pragma once
#include "../../../common/reservation_allocator.h"

// Preserve engine-private spelling; engine-independent supplied values can use
// the same allocation mechanism without depending on an execution component.
namespace assembly::detail {
using hcp::resources::ReservationBudget;
using hcp::resources::ReservationStatus;
using hcp::resources::ReservationResult;
using hcp::resources::ReservationStop;
using hcp::resources::ReservationDomain;
using hcp::resources::ReservationAllocator;
using hcp::resources::ReservedVector;
using hcp::resources::SharedReservationAllocator;
using hcp::resources::SharedReservedVector;
using hcp::resources::ReservationDelete;
using hcp::resources::ReservedUnique;
using hcp::resources::reserve_unique;
}