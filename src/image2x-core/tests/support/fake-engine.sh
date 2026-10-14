#!/bin/sh
# This executable is immutable during tests. Fixture-specific code is read as
# shell data, so an inherited writable data descriptor cannot block exec.
. ./fake-engine-behavior
