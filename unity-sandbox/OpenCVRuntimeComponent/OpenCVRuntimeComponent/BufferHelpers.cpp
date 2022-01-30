//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.h"
#include "BufferHelpers.h"

namespace OpenCVRuntimeComponent
{
    namespace Io
    {
        void* GetPointerToMemoryBuffer(
            _In_ Windows::Foundation::IMemoryBufferReference^ memoryBuffer,
            _Out_ uint32_t& memoryBufferLength)
        {
            //
            // Query the IBufferByteAccess interface.
            //
            Microsoft::WRL::ComPtr<Windows::Foundation::IMemoryBufferByteAccess> bufferByteAccess;

            reinterpret_cast<IInspectable*>(memoryBuffer)->QueryInterface(
                IID_PPV_ARGS(
                    &bufferByteAccess));

            //
            // Retrieve the buffer data.
            //
            uint8_t* memoryBufferData = nullptr;

            bufferByteAccess->GetBuffer(
                &memoryBufferData,
                &memoryBufferLength);

            return memoryBufferData;
        }

        void* GetPointerToIBuffer(
            Windows::Storage::Streams::IBuffer^ buffer)
        {
            byte* rawData = nullptr;
            if (nullptr != buffer && 0 < buffer->Length)
            {
                Microsoft::WRL::ComPtr<IUnknown> bufferAsIUnknown =
                    reinterpret_cast<IUnknown*>(buffer);

                Microsoft::WRL::ComPtr<Windows::Storage::Streams::IBufferByteAccess> bufferByteAccess;

                bufferAsIUnknown.As(
                    &bufferByteAccess);

                bufferByteAccess->Buffer(&rawData);
            }
            return rawData;
        }
    }
}




