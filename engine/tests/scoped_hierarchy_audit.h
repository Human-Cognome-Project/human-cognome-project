#pragma once

namespace field {
class Harness;
}

void audit_scoped_hierarchy(field::Harness &harness, bool scoped_responses);
void scoped_audit_no_unroll();
