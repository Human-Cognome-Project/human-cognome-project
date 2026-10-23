// Compatibility probe: deliberately uses only the pre-v2 public API. Compile
// this same source against the archived v1 headers/library to test a real old
// reader. It is an isolated fixture test, never an operational reader service.
#include <cstdlib>
#include <iostream>
#include <string>

#include <warm/pg.h>
#include <warm/toolbox.h>

int main(int argc, char **argv) {
  try {
    if (argc != 2 || !std::getenv("HCP_WARM_TEST_CONNINFO"))
      return 2;
    const std::string conn = std::getenv("HCP_WARM_TEST_CONNINFO");
    warm::detail::Pg sql(conn);
    auto r = sql.query("SELECT selection,revision,owner,creator FROM "
                       "warm_toolbox.bundle_revision "
                       "WHERE recipe_version=1 ORDER BY selection LIMIT 1");
    if (PQntuples(r.get()) != 1)
      throw warm::Error("missing original fixture");
    const warm::BundleRef ref{warm::Uuid(warm::detail::cell(r, 0, 0)),
                              warm::Uuid(warm::detail::cell(r, 0, 1))};
    const warm::Context context{warm::Uuid(warm::detail::cell(r, 0, 3)),
                                warm::Uuid(warm::detail::cell(r, 0, 2)),
                                warm::WriterRole::Reader};
    warm::Toolbox reader(conn, context);
    r = sql.query("SELECT consumer FROM warm_toolbox.bundle_lease WHERE "
                  "selection=$1 AND bundle_revision=$2",
                  warm::detail::params(ref));
    if (PQntuples(r.get()) != 1)
      throw warm::Error("missing retained v1 pin");
    auto old = reader.read_pinned(ref, warm::Uuid(warm::detail::cell(r, 0, 0)));
    if (old.bundle.recipe_version != 1 || old.bundle.occurrences.size() != 4 ||
        old.bundle.occurrences[0].position.x != -2)
      throw warm::Error("v1 pin changed");
    const auto consumer = warm::new_uuid();
    const bool expect_rejection = std::string(argv[1]) == "reject-v2";
    try {
      auto current = reader.acquire(ref.selection, consumer);
      if (expect_rejection || current.bundle.recipe_version != 2)
        throw std::runtime_error(
            "reader accepted an unexpected recipe version");
      reader.release(current.bundle.ref, consumer);
    } catch (const warm::Error &e) {
      if (!expect_rejection ||
          std::string(e.what()) != "unsupported recipe version")
        throw;
      auto count = sql.query(
          "SELECT count(*) FROM warm_toolbox.bundle_lease WHERE consumer=$1",
          {consumer.str()});
      if (warm::detail::cell(count, 0, 0) != "0")
        throw warm::Error("rejected read acquired a pin");
    }
    std::cout << "PASS: retained v1 pin exact; v2 head "
              << (expect_rejection ? "refused cleanly without acquiring a lease"
                                   : "read successfully")
              << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
