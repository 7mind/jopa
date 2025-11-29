#!/usr/bin/env bash
# Wrapper for zip to fix glibj.zip build issue
# Info-ZIP reads ZIP env var as default archive name, causing it to try
# to open this wrapper script as a zip file. Unset it before calling zip.

unset ZIP
exec zip "$@"
