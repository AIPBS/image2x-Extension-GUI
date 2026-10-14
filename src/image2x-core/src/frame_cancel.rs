//! Frame-only cancellation reservations and completion/cancel race ordering.

use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

static ACTIVE: OnceLock<Mutex<HashMap<String, Arc<AtomicBool>>>> = OnceLock::new();

pub(crate) struct Cancellation {
    id: Option<String>,
    flag: Arc<AtomicBool>,
}

impl Cancellation {
    pub(crate) fn local() -> Self {
        Self {
            id: None,
            flag: Arc::new(AtomicBool::new(false)),
        }
    }

    pub(crate) fn register(id: &str) -> Result<Self, String> {
        // Hold the registry mutex only for registration/cancel/completion, never
        // across process execution or filesystem work. A separate socket can
        // therefore cancel a job even while its original client is blocked.
        let mut active = ACTIVE
            .get_or_init(Default::default)
            .lock()
            .map_err(|_| "frame cancellation registry is poisoned")?;
        if active.contains_key(id) {
            return Err(format!("frame request id is already active: {id}"));
        }
        let mut token = Self::local();
        token.id = Some(id.to_owned());
        active.insert(id.to_owned(), Arc::clone(&token.flag));
        Ok(token)
    }

    pub(crate) fn is_cancelled(&self) -> bool {
        self.flag.load(Ordering::Acquire)
    }

    pub(crate) fn complete(&mut self) -> bool {
        // Completion and cancellation use the same mutex. Once success commits
        // here, cancel returns false. If cancel wins first, even a child that
        // already exited successfully must have its output rejected and cleaned.
        if let Some(id) = self.id.take() {
            if let Some(registry) = ACTIVE.get() {
                if let Ok(mut active) = registry.lock() {
                    active.remove(&id);
                    return self.is_cancelled();
                }
            }
        }
        self.is_cancelled()
    }
}

impl Drop for Cancellation {
    fn drop(&mut self) {
        // Every return path, including validation failure, releases this ID.
        self.complete();
    }
}

pub(crate) fn cancel(target_id: &str) -> bool {
    let Some(registry) = ACTIVE.get() else {
        return false;
    };
    let Ok(active) = registry.lock() else {
        return false;
    };
    let Some(flag) = active.get(target_id) else {
        return false;
    };
    flag.store(true, Ordering::Release);
    true
}

#[cfg(test)]
mod tests {
    use super::{cancel, Cancellation};

    #[test]
    fn cancellation_before_spawn_and_completion_races_are_ordered() {
        let mut token = Cancellation::register("cancel-before-spawn").unwrap();
        assert!(cancel("cancel-before-spawn"));
        assert!(token.is_cancelled());
        assert!(token.complete());
        assert!(!cancel("cancel-before-spawn"));

        let mut token = Cancellation::register("complete-before-cancel").unwrap();
        assert!(!token.complete());
        assert!(!cancel("complete-before-cancel"));
    }

    #[test]
    fn duplicate_ids_are_rejected_and_drop_releases_registration() {
        let token = Cancellation::register("duplicate-frame-id").unwrap();
        assert!(Cancellation::register("duplicate-frame-id").is_err());
        drop(token);
        assert!(!cancel("duplicate-frame-id"));
        assert!(Cancellation::register("duplicate-frame-id").is_ok());
    }
}
