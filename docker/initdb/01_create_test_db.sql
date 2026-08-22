-- Create the integration-test database alongside the dev database.
-- Executed by the postgres image entrypoint on FIRST initialization of an
-- empty data volume only (wipe with scripts/db-down.sh --wipe to re-run).
CREATE DATABASE modulo_test OWNER modulo;
