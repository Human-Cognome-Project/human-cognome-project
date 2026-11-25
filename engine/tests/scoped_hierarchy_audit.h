#pragma once

namespace field {
class Harness;
}

void audit_scoped_hierarchy(field::Harness &harness);
void scoped_audit_no_unroll();
