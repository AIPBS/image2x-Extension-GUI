//! Immutable fake-executable publication for concurrently spawning Linux tests.

use std::fs;
use std::io;
use std::path::Path;

pub fn install(program: &Path, behavior: &str) -> io::Result<()> {
    let parent = program.parent().ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::InvalidInput,
            "fake executable requires a parent directory",
        )
    })?;
    // A concurrent fork can retain another thread's writable descriptor even
    // after fs::write closes the parent copy. CLOEXEC releases that child copy
    // only at exec, so a freshly written executable can spuriously get ETXTBSY.
    // Never write an executable inode here. The launcher is immutable source;
    // per-fixture behavior is data read by the shell, not executed by the kernel.
    // No lock serializes test setup or process spawning.
    fs::write(parent.join("fake-engine-behavior"), behavior)?;
    let launcher = Path::new(env!("CARGO_MANIFEST_DIR")).join("tests/support/fake-engine.sh");
    std::os::unix::fs::symlink(launcher, program)
}
