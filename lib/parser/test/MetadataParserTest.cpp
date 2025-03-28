// Copyright 2023-2025 Emmanuel Chaboud
//
/// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cxximg/parser/MetadataParser.h"
#include "cxximg/util/File.h"

#include <gtest/gtest.h>

using namespace cxximg;

TEST(MetadataParserTest, TestSerializationFull) {
    ImageMetadata metadata = {
            .fileInfo = {.fileFormat = FileFormat::PLAIN,
                         .pixelRepresentation = PixelRepresentation::UINT16,
                         .imageLayout = ImageLayout::PLANAR,
                         .pixelType = PixelType::BAYER_RGGB,
                         .pixelPrecision = 12,
                         .width = 4000,
                         .height = 3000,
                         .widthAlignment = 1,
                         .heightAlignment = 1,
                         .sizeAlignment = 1},
            .exifMetadata = {.imageWidth = 4000,
                             .imageHeight = 3000,
                             .imageDescription = "My description",
                             .make = "Make",
                             .model = "Model",
                             .orientation = 1,
                             .software = "",
                             .exposureTime = ExifMetadata::Rational{1, 100},
                             .fNumber = ExifMetadata::Rational{56, 10},
                             .isoSpeedRatings = 100,
                             .dateTimeOriginal = "2023:08:25 17:13:31",
                             .brightnessValue = ExifMetadata::SRational{25, 10},
                             .exposureBiasValue = ExifMetadata::SRational{-100, 100},
                             .focalLength = ExifMetadata::Rational{35, 1},
                             .focalLengthIn35mmFilm = 50,
                             .lensMake = "Lens make",
                             .lensModel = "Lens model"},
            .shootingParams = {.aperture = 5.6f,
                               .exposureTime = 0.01f,
                               .sensitivity = 100.0f,
                               .totalGain = 1.0f,
                               .sensorGain = 1.0f,
                               .ispGain = 1.0f,
                               .zoom = Rectf{0.05f, 0.1f, 0.9f, 0.8f}},
            .calibrationData = {.blackLevel = 64,
                                .whiteLevel = 1024.0f,
                                .vignetting = DynamicMatrix{{3.0f, 1.5f, 3.0f}, {1.5f, 1.0f, 1.5f}, {3.0f, 1.5f, 3.0f}},
                                .colorMatrix = Matrix3{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                                .colorMatrixTarget = RgbColorSpace::SRGB},
            .cameraControls = {.whiteBalance = ImageMetadata::WhiteBalance{2.0f, 1.5},
                               .colorShading = ImageMetadata::ColorShading{{{1.2f, 1.05f, 1.2f},
                                                                            {1.05f, 1.0f, 1.05f},
                                                                            {1.2f, 1.05f, 1.2f}},
                                                                           {{0.95f, 0.99f, 0.95f},
                                                                            {0.99f, 1.0f, 0.99f},
                                                                            {0.95f, 0.99f, 0.95f}}},
                               .faceDetection = std::vector<Rectf>{{0.1f, 0.15f, 0.2f, 0.3f}}},
            .semanticMasks = {}};

    parser::writeMetadata(metadata, "test_serialization_full.json");
    EXPECT_NO_THROW(parser::readMetadata("test_serialization_full.json"));

    std::string json = file::readContent("test_serialization_full.json");
    const char* ref = R"V0G0N({
    "fileInfo": {
        "fileFormat": "plain",
        "pixelRepresentation": "uint16",
        "imageLayout": "planar",
        "pixelType": "bayer_rggb",
        "pixelPrecision": 12,
        "width": 4000,
        "height": 3000,
        "widthAlignment": 1,
        "heightAlignment": 1,
        "sizeAlignment": 1
    },
    "exifMetadata": {
        "imageWidth": 4000,
        "imageHeight": 3000,
        "imageDescription": "My description",
        "make": "Make",
        "model": "Model",
        "orientation": 1,
        "software": "",
        "exposureTime": [
            1,
            100
        ],
        "fNumber": [
            56,
            10
        ],
        "isoSpeedRatings": 100,
        "dateTimeOriginal": "2023:08:25 17:13:31",
        "brightnessValue": [
            25,
            10
        ],
        "exposureBiasValue": [
            -100,
            100
        ],
        "focalLength": [
            35,
            1
        ],
        "focalLengthIn35mmFilm": 50,
        "lensMake": "Lens make",
        "lensModel": "Lens model"
    },
    "shootingParams": {
        "aperture": 5.599999904632568,
        "exposureTime": 0.009999999776482582,
        "sensitivity": 100.0,
        "totalGain": 1.0,
        "sensorGain": 1.0,
        "ispGain": 1.0,
        "zoom": [
            0.05000000074505806,
            0.10000000149011612,
            0.8999999761581421,
            0.800000011920929
        ]
    },
    "calibrationData": {
        "blackLevel": 64,
        "whiteLevel": 1024.0,
        "vignetting": [
            [
                3.0,
                1.5,
                3.0
            ],
            [
                1.5,
                1.0,
                1.5
            ],
            [
                3.0,
                1.5,
                3.0
            ]
        ],
        "colorMatrix": [
            [
                1.0,
                0.0,
                0.0
            ],
            [
                0.0,
                1.0,
                0.0
            ],
            [
                0.0,
                0.0,
                1.0
            ]
        ],
        "colorMatrixTarget": "srgb"
    },
    "cameraControls": {
        "whiteBalance": [
            2.0,
            1.5
        ],
        "colorShading": [
            [
                [
                    1.2000000476837158,
                    1.0499999523162842,
                    1.2000000476837158
                ],
                [
                    1.0499999523162842,
                    1.0,
                    1.0499999523162842
                ],
                [
                    1.2000000476837158,
                    1.0499999523162842,
                    1.2000000476837158
                ]
            ],
            [
                [
                    0.949999988079071,
                    0.9900000095367432,
                    0.949999988079071
                ],
                [
                    0.9900000095367432,
                    1.0,
                    0.9900000095367432
                ],
                [
                    0.949999988079071,
                    0.9900000095367432,
                    0.949999988079071
                ]
            ]
        ],
        "faceDetection": [
            [
                0.10000000149011612,
                0.15000000596046448,
                0.20000000298023224,
                0.30000001192092896
            ]
        ]
    },
    "semanticMasks": []
})V0G0N";

    ASSERT_EQ(ref, json);
}

TEST(MetadataParserTest, TestDeserializationPartial) {
    std::ofstream ofs("test_deserialization_partial.json");
    ofs << R"V0G0N({
    "fileInfo": {
        "fileFormat": "plain",
        "pixelRepresentation": "uint16",
        "imageLayout": "planar",
        "pixelType": "bayer_rggb",
        "pixelPrecision": 12,
        "width": 4000,
        "height": 3000,
        "widthAlignment": 1
    }
})V0G0N";
    ofs.close();

    ImageMetadata parsed = parser::readMetadata("test_deserialization_partial.json");
    parser::writeMetadata(parsed, "test_deserialization_partial.json");

    std::string json = file::readContent("test_deserialization_partial.json");
    const char* ref = R"V0G0N({
    "fileInfo": {
        "fileFormat": "plain",
        "pixelRepresentation": "uint16",
        "imageLayout": "planar",
        "pixelType": "bayer_rggb",
        "pixelPrecision": 12,
        "width": 4000,
        "height": 3000,
        "widthAlignment": 1
    },
    "exifMetadata": {},
    "shootingParams": {},
    "calibrationData": {},
    "cameraControls": {},
    "semanticMasks": []
})V0G0N";

    ASSERT_EQ(ref, json);
}
