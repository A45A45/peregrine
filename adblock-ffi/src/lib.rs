use adblock::engine::Engine;
use adblock::lists::FilterSet;
use adblock::request::Request;
use std::ffi::CStr;
use std::os::raw::{c_char, c_int};
use std::sync::Mutex;
use once_cell::sync::Lazy;

struct EngineWrapper(Engine);
unsafe impl Send for EngineWrapper {}
unsafe impl Sync for EngineWrapper {}

static ENGINE: Lazy<Mutex<Option<EngineWrapper>>> = Lazy::new(|| Mutex::new(None));

#[no_mangle]
pub extern "C" fn adblock_init_engine(filter_lists: *const *const c_char, count: c_int) -> c_int {
    if filter_lists.is_null() || count <= 0 {
        return -1;
    }

    let mut filter_set = FilterSet::new(false);

    for i in 0..count {
        unsafe {
            let ptr = *filter_lists.offset(isize::try_from(i).unwrap());
            if !ptr.is_null() {
                let c_str = CStr::from_ptr(ptr);
                if let Ok(s) = c_str.to_str() {
                    filter_set.add_filter_list(s.to_string(), Default::default());
                }
            }
        }
    }

    let engine = Engine::new_with_filter_set(filter_set);

    if let Ok(mut eng) = ENGINE.lock() {
        *eng = Some(EngineWrapper(engine));
        0
    } else {
        -1
    }
}

#[no_mangle]
pub extern "C" fn adblock_should_block(
    url: *const c_char,
    source_url: *const c_char,
    request_type: *const c_char,
) -> c_int {
    if url.is_null() {
        return 0;
    }

    let url_str = unsafe { CStr::from_ptr(url).to_str().unwrap_or("") };
    let source_str = unsafe {
        if source_url.is_null() {
            ""
        } else {
            CStr::from_ptr(source_url).to_str().unwrap_or("")
        }
    };
    let type_str = unsafe {
        if request_type.is_null() {
            "other"
        } else {
            CStr::from_ptr(request_type).to_str().unwrap_or("other")
        }
    };

    if let Ok(eng) = ENGINE.lock() {
        if let Some(ref wrapper) = *eng {
            if let Ok(request) = Request::new(url_str, source_str, type_str, "GET") {
                let result = wrapper.0.check_network_request(&request);
                let blocked = result.exception.is_none() && result.filter.is_some();
                return if blocked { 1 } else { 0 };
            }
        }
    }

    0
}
