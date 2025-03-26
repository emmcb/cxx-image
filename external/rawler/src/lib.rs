use rawler::decoders::RawDecodeParams;
use rawler::imgop::xyz::Illuminant;
use rawler::rawsource::RawSource;
use std::ffi::c_void;
use std::os::raw::{c_char, c_uchar, c_uint};
use std::slice;

// Convert a Rust string to a fixed-size C char array
fn string_to_fixed_c_chars(s: &str, out: &mut [c_char; 32]) {
    let cstring = std::ffi::CString::new(s).unwrap_or_default();
    let bytes = cstring.as_bytes_with_nul();
    let copy_len = std::cmp::min(bytes.len(), out.len());

    // Copy bytes including null terminator
    for i in 0..copy_len {
        out[i] = bytes[i] as c_char;
    }

    // Ensure null termination if string was too long
    if copy_len == out.len() {
        out[out.len() - 1] = 0;
    }
}

// Convert a Rust string to a C string
fn string_to_c_char(s: &str) -> *mut c_char {
    let cstring = std::ffi::CString::new(s).unwrap_or_default();
    cstring.into_raw()
}

// Re-export RawImageData enum as a C-compatible struct
#[repr(C)]
pub enum DataType {
    Integer = 0,
    Float = 1,
}

#[repr(C)]
pub struct Exif {
    orientation: u16,
    exposure_time: [u32; 2],
    fnumber: [u32; 2],
    iso_speed_ratings: u16,
    date_time_original: [c_char; 32],
    brightness_value: [i32; 2],
    exposure_bias: [i32; 2],
    focal_length: [u32; 2],
}

#[repr(C)]
pub struct RawMetadata {
    exif: Exif,
    model: [c_char; 32],
    make: [c_char; 32],
}

// Combined struct for raw image data and metadata
#[repr(C)]
pub struct RawImage {
    /// width of the full image
    width: c_uint,
    /// height of the full image
    height: c_uint,
    /// number of components per pixel (1 for bayer, 3 for RGB images)
    cpp: c_uint,
    /// bits per pixel
    bps: c_uint,
    /// cfa pattern as a string
    cfa: [c_char; 32],
    /// image black level
    black_levels: [f32; 4],
    /// image white levels
    white_levels: [f32; 4],
    /// whitebalance coefficients encoded in the file in RGBE order
    wb_coeffs: [f32; 4],
    /// color matrix
    color_matrix: [f32; 9],
    /// image metadata
    metadata: RawMetadata,
    /// image data type
    data_type: DataType,
    /// image data pointer
    data_ptr: *const c_void,
    /// image data length
    data_len: usize,
}

// Main function to decode raw image from a buffer
#[no_mangle]
pub unsafe extern "C" fn decode_buffer(
    buffer: *const c_uchar,
    buffer_size: usize,
    error_msg: *mut *mut c_char,
) -> *mut RawImage {
    // Set default error and result
    let mut result = std::ptr::null_mut();

    // Input validation
    if buffer.is_null() || buffer_size == 0 {
        if !error_msg.is_null() {
            *error_msg = string_to_c_char("Empty buffer provided");
        }
        return result;
    }

    let data_slice = slice::from_raw_parts(buffer, buffer_size);
    let buf = RawSource::new_from_slice(data_slice);
    let params = RawDecodeParams::default();

    let decode_result = std::panic::catch_unwind(|| {
        // Handle each operation manually with proper error conversion
        let decoder = match rawler::get_decoder(&buf) {
            Ok(decoder) => decoder,
            Err(err) => return Err(format!("Failed to get decoder: {}", err)),
        };

        let raw_image = match decoder.raw_image(&buf, &params, false) {
            Ok(img) => img,
            Err(err) => return Err(format!("Failed to decode raw image: {}", err)),
        };

        // Process the image data
        let (data_type, data_ptr, data_len) = match raw_image.data {
            rawler::RawImageData::Integer(data) => {
                let len = data.len();
                let ptr = data.as_ptr();
                std::mem::forget(data);
                (DataType::Integer, ptr as *const c_void, len)
            }
            rawler::RawImageData::Float(data) => {
                let len = data.len();
                let ptr = data.as_ptr();
                std::mem::forget(data);
                (DataType::Float, ptr as *const c_void, len)
            }
        };

        // Create the combined decoded image struct
        let mut decoded_image = Box::new(RawImage {
            width: raw_image.width as c_uint,
            height: raw_image.height as c_uint,
            cpp: raw_image.cpp as c_uint,
            bps: raw_image.bps as c_uint,
            cfa: [0; 32],
            black_levels: raw_image.blacklevel.as_bayer_array(),
            white_levels: raw_image.whitelevel.as_bayer_array(),
            wb_coeffs: raw_image.wb_coeffs,
            color_matrix: [0.0; 9],
            metadata: RawMetadata {
                exif: Exif {
                    orientation: 0,
                    exposure_time: [0; 2],
                    fnumber: [0; 2],
                    iso_speed_ratings: 0,
                    date_time_original: [0; 32],
                    brightness_value: [0; 2],
                    exposure_bias: [0; 2],
                    focal_length: [0; 2],
                },
                model: [0; 32],
                make: [0; 32],
            },
            data_type,
            data_ptr,
            data_len,
        });

        // Fill in the cfa pattern
        string_to_fixed_c_chars(&raw_image.camera.cfa.name, &mut decoded_image.cfa);

        // Fill in the color matrix
        if let Some(color_matrix) = raw_image.color_matrix.get(&Illuminant::D65) {
            for i in 0..9 {
                decoded_image.color_matrix[i] = color_matrix[i];
            }
        }

        // Fill in the metadata
        if let Ok(metadata) = decoder.raw_metadata(&buf, &params) {
            // Copy make and model to metadata
            string_to_fixed_c_chars(&metadata.make, &mut decoded_image.metadata.make);
            string_to_fixed_c_chars(&metadata.model, &mut decoded_image.metadata.model);

            if let Some(orientation) = metadata.exif.orientation {
                decoded_image.metadata.exif.orientation = orientation;
            }
            if let Some(exposure_time) = metadata.exif.exposure_time {
                decoded_image.metadata.exif.exposure_time = [exposure_time.n, exposure_time.d];
            }
            if let Some(fnumber) = metadata.exif.fnumber {
                decoded_image.metadata.exif.fnumber = [fnumber.n, fnumber.d];
            }
            if let Some(iso_speed_ratings) = metadata.exif.iso_speed_ratings {
                decoded_image.metadata.exif.iso_speed_ratings = iso_speed_ratings;
            }
            if let Some(date_time_original) = metadata.exif.date_time_original {
                string_to_fixed_c_chars(
                    &date_time_original,
                    &mut decoded_image.metadata.exif.date_time_original,
                );
            }
            if let Some(brightness_value) = metadata.exif.brightness_value {
                decoded_image.metadata.exif.brightness_value =
                    [brightness_value.n, brightness_value.d];
            }
            if let Some(exposure_bias) = metadata.exif.exposure_bias {
                decoded_image.metadata.exif.exposure_bias = [exposure_bias.n, exposure_bias.d];
            }
            if let Some(focal_length) = metadata.exif.focal_length {
                decoded_image.metadata.exif.focal_length = [focal_length.n, focal_length.d];
            }
        }

        Ok(Box::into_raw(decoded_image))
    });

    // Handle the result of decoding
    if !error_msg.is_null() {
        *error_msg = match decode_result {
            Ok(Ok(image)) => {
                result = image;
                std::ptr::null_mut()
            }
            Ok(Err(err)) => string_to_c_char(&err),
            Err(_) => string_to_c_char("Panic occurred during decoding"),
        };
    } else if let Ok(Ok(image)) = decode_result {
        result = image;
    }

    result
}

// Free raw decoded image allocated by Rust
#[no_mangle]
pub unsafe extern "C" fn free_image(decoded_image: *mut RawImage) {
    if !decoded_image.is_null() {
        let decoded_image = Box::from_raw(decoded_image);

        // Free the image data
        match decoded_image.data_type {
            DataType::Integer => {
                drop(Vec::from_raw_parts(
                    decoded_image.data_ptr as *mut u16,
                    decoded_image.data_len,
                    decoded_image.data_len,
                ));
            }
            DataType::Float => {
                drop(Vec::from_raw_parts(
                    decoded_image.data_ptr as *mut f32,
                    decoded_image.data_len,
                    decoded_image.data_len,
                ));
            }
        }
    }
}
