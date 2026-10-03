-- Structural-mass trigger — additive install migration.
--
-- Installs the one authorized trigger exception to "no logic in this schema"
-- on an EXISTING hcp_core: CREATE FUNCTION + CREATE TRIGGER only. No data is
-- touched; the trigger fires only on NEW token_parent inserts, so seeded
-- floor rows keep their stored masses. The same DDL lives in schema.sql for
-- fresh databases; keep the two identical.
--
-- On each token_parent insert:
--   1. Guard (#109, direct-mint path): if the just-linked parent's
--      token.mass is NULL, RAISE -- an incomplete parent is not available
--      for composition. SUM must never silently skip a NULL parent.
--   2. Structural mass: child's token.mass := SUM of its direct parents'
--      token.mass. One level only (each parent's stored mass already
--      aggregates its own subtree); no cascade to descendants.

CREATE FUNCTION token_parent_structural_mass() RETURNS trigger
LANGUAGE plpgsql AS $$
DECLARE
    parent_mass integer;
BEGIN
    SELECT mass INTO parent_mass FROM token WHERE token_id = NEW.parent_token_id;
    IF parent_mass IS NULL THEN
        RAISE EXCEPTION
            'parent % has unknown (NULL) mass: not available for composition',
            NEW.parent_token_id;
    END IF;

    UPDATE token
    SET mass = (SELECT SUM(p.mass)::integer
                FROM token_parent tp
                JOIN token p ON p.token_id = tp.parent_token_id
                WHERE tp.token_id = NEW.token_id)
    WHERE token_id = NEW.token_id;
    RETURN NULL;
END;
$$;

CREATE TRIGGER token_parent_structural_mass
AFTER INSERT ON token_parent
FOR EACH ROW EXECUTE FUNCTION token_parent_structural_mass();
