// Adapted from Rene Schulte's repo 
// https://github.com/microsoft/Windows-Machine-Learning/tree/master/Samples/MNIST

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.XR.WSA;
using UnityEngine.XR.WSA.Input;

#if ENABLE_WINMD_SUPPORT
using Windows.Graphics.Imaging;
using Windows.Perception.Spatial;
#endif

public class NetworkBehaviour : MonoBehaviour
{
    
    /// <summary>
    /// Media capture profile selection - recommend 896 x 504
    /// </summary>
    public MediaCaptureUtility.MediaCaptureProfiles MediaCaptureProfiles;

    /// <summary>
    /// Selection of desired ArUco dictionary for marker tracking and the tracking type
    /// </summary>
    public ArUcoUtils.ArUcoDictionaryName ArUcoDictionaryName = ArUcoUtils.ArUcoDictionaryName.DICT_6X6_50;
    public ArUcoUtils.ArUcoTrackingType ArUcoTrackingType = ArUcoUtils.ArUcoTrackingType.Markers;
    public ArUcoUtils.ArUcoTrackingType ArUcoTrackingTypeAfterCalibration = ArUcoUtils.ArUcoTrackingType.CustomBoard;

    /// <summary>
    /// Handle use of user-defined calibration paramters OR per-frame calibration data
    /// </summary>
    public ArUcoUtils.CameraCalibrationParameterType CalibrationParameterType = ArUcoUtils.CameraCalibrationParameterType.PerFrame;

    /// <summary>
    /// Instance of the tracking game objects class to handle
    /// control of the game objects for marker and board tracking.
    /// </summary>
    public TrackingGos TrackingGos;

    /// <summary>
    /// Handle the use of user-defined HMD transform OR use point-based calibration
    /// </summary>
    public ArUcoUtils.HMDCalibrationType HMDCalibrationType = ArUcoUtils.HMDCalibrationType.UserDefined;
    private ArUcoUtils.HMDCalibrationStatus _HMDCalibrationStatus = ArUcoUtils.HMDCalibrationStatus.StartedCalibration;

    /// <summary>
    /// The desired moving average length for cached marker transform values.
    /// </summary>
    public int NumMovingAvgPts = 1;

    /// <summary>
    /// Attach collect point correspondences script if running calibration
    /// </summary>
    public ArUcoDetectionHoloLensUnity.CollectPointCorrespondences CollectPointCorrespondences;

    /// <summary>
    /// Public text blocks for status updates and FPS.
    /// </summary>
    public Text StatusBlock;

    /// <summary>
    /// Media capture utility class instance for handling media frame source groups.
    /// </summary>
    private MediaCaptureUtility _MediaCaptureUtility;
    private bool _isRunning = false;

    /// <summary>
    /// Instance of the aruco board positions script
    /// to define the marker size and parameters for custom board tracking
    /// </summary>
    public ArUcoBoardPositions ArUcoBoardPositions;

    /// <summary>
    /// Predefined transforms for testing registration
    /// methods without having to redo point calibrations
    /// </summary>
    public PredefinedTransform PredefinedTransform;

    /// <summary>
    /// Holder for the camera parameters (intrinsics and extrinsics)
    /// of the tracking sensor on the HoloLens 2
    /// </summary>
    public UserDefinedCameraCalibrationParams UserDefinedCalibParams;

    /// <summary>
    /// Boolean value to handle world anchor status
    /// </summary>
    private bool _isWorldAnchored = false;

    /// <summary>
    /// Property fields to cache transforms for HMD calibration.
    /// </summary>
    public Matrix4x4 TransformUnityCamera { get; set; }
    public Matrix4x4 CameraToWorldUnity { get; set; }

    /// <summary>
    /// Gesture handler for fast debugging.
    /// </summary>
    GestureRecognizer _gestureRecognizerDoubleTap;

#if ENABLE_WINMD_SUPPORT
    /// <summary>
    /// OpenCV windows runtime dll component
    /// </summary>
    OpenCVRuntimeComponent.CvUtils CvUtils;

    /// <summary>
    /// Coordinate system reference for Unity to WinRt transform construction.
    /// </summary>
    private SpatialCoordinateSystem _unityCoordinateSystem = null;
    private SpatialCoordinateSystem _frameCoordinateSystem = null;

#endif

    // Queue of points for improving the stability of head-relative 
    // marker pose measures
    private Queue<Vector3> _posCamQ = new Queue<Vector3>();
    private Queue<Quaternion> _rotCamQ = new Queue<Quaternion>();

    #region UnityMethods
    async void Start()
    {
        // Initialize the gesture handler
        InitializeHandler();

        try
        {
#if ENABLE_WINMD_SUPPORT

            // Set the HMD calibration status from calibration type
            switch (HMDCalibrationType)
            {
                case ArUcoUtils.HMDCalibrationType.UserDefined:
                    CollectPointCorrespondences.HideCalibrationReticle();
                    _HMDCalibrationStatus = ArUcoUtils.HMDCalibrationStatus.NotCalibrating;
                    Debug.Log("Start: HMDCalibrationType: User Defined, not calibrating.");
                    break;

                case ArUcoUtils.HMDCalibrationType.PointBased:
                    // Initialize the point correspondences class
                    CollectPointCorrespondences.Initialize();

                    switch (_HMDCalibrationStatus)
                    {
                        case ArUcoUtils.HMDCalibrationStatus.NotCalibrating:
                            CollectPointCorrespondences.HideCalibrationReticle();
                            PredefinedTransform.UserDefinedTransformLeftEye = Matrix4x4.identity;
                            PredefinedTransform.UserDefinedTransformRightEye = Matrix4x4.identity;
                            Debug.Log("Start: HMDCalibrationType: not calibrating.");
                            break;
                        
                        case ArUcoUtils.HMDCalibrationStatus.StartedCalibration:
                            ArUcoTrackingType = ArUcoUtils.ArUcoTrackingType.Markers;
                            PredefinedTransform.UserDefinedTransformLeftEye = Matrix4x4.identity;
                            PredefinedTransform.UserDefinedTransformRightEye = Matrix4x4.identity;
                            Debug.Log("Start: HMDCalibrationType: started calibration.");
                            break;
                        
                        case ArUcoUtils.HMDCalibrationStatus.CompletedCalibration:
                            CollectPointCorrespondences.HideCalibrationReticle();
                            ArUcoTrackingType = ArUcoTrackingTypeAfterCalibration;
                            Debug.Log("Start: HMDCalibrationType: completed calibration.");
                            break;
                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }

            // Asynchronously start media capture
            await StartMediaCapture();

            // Configure the dll with input parameters
            CvUtils = new OpenCVRuntimeComponent.CvUtils(
                ArUcoBoardPositions.ComputeMarkerSizeForTrackingType(
                    ArUcoTrackingType, 
                    ArUcoBoardPositions.markerSizeForSingle,
                    ArUcoBoardPositions.markerSizeForBoard),
                ArUcoBoardPositions.numMarkers,
                (int)ArUcoDictionaryName,
                ArUcoBoardPositions.FillCustomObjectPointsFromUnity());
            Debug.Log("Created new instance of the cvutils class.");

            // Run processing loop in separate parallel Task, get the latest frame
            // and asynchronously evaluate
            Debug.Log("Begin tracking in frame grab loop.");
            _isRunning = true;

            // Initialize game objects
            HandleGoIntitialization();

            // Run the frame grab and aruco tracking in a new task block
            await Task.Run(() =>
            {
                while (_isRunning)
                {
                    if (_MediaCaptureUtility.IsCapturing)
                    {
                        var mediaFrameReference = _MediaCaptureUtility.GetLatestFrame();
                        HandleArUcoTracking(mediaFrameReference);
                        mediaFrameReference?.Dispose();
                    }
                    else
                    {
                        return;
                    }
                }
            });
#endif 
        }
        catch (Exception ex)
        {
            StatusBlock.text = $"Error init: {ex.Message}";
            Debug.LogError($"Failed to start marker tracking: {ex}");
        }
    }

    private void Update()
    {
#if ENABLE_WINMD_SUPPORT
        // register the space key for point collection on HL2
        // when connected as an external device.
        if (Input.GetKeyDown("space"))
        {
            Debug.Log("Registered space bar press.");
            ControlCalib();
        }
#endif
    }

    private async void OnDestroy()
    {
        _isRunning = false;
        if (_MediaCaptureUtility != null)
        {
            await _MediaCaptureUtility.StopMediaFrameReaderAsync();
        }

        // Dispose of gesture handler
        _gestureRecognizerDoubleTap.StopCapturingGestures();
        _gestureRecognizerDoubleTap.Dispose();
    }
    #endregion

    #region TapGestureHandler
    [Obsolete]
    private void InitializeHandler()
    {
        // New recognizer class
        _gestureRecognizerDoubleTap = new GestureRecognizer();

        // Set tap as a recognizable gesture
        _gestureRecognizerDoubleTap.SetRecognizableGestures(GestureSettings.DoubleTap);

        // Begin listening for gestures
        _gestureRecognizerDoubleTap.StartCapturingGestures();

        // Capture on gesture events with delegate handler
        _gestureRecognizerDoubleTap.Tapped += GestureRecognizer_DoubleTapped;

        Debug.Log("Gesture recognizer initialized.");
    }

    private void GestureRecognizer_DoubleTapped(TappedEventArgs obj)
    {
        // Collect point correspondence
        Debug.Log("Double tap gesture detected.");
#if ENABLE_WINMD_SUPPORT
        ControlCalib();
#endif
    }
    #endregion

    /// <summary>
    /// Handle media capture utility instantiation and caching of
    /// the Unity coordinate system.
    /// </summary>
    /// <returns></returns>
    private async Task StartMediaCapture()
    {
        StatusBlock.text = $"Starting camera...";

#if ENABLE_WINMD_SUPPORT
        // Configure camera to return frames fitting the model input size
        try
        {
            Debug.Log("Creating MediaCaptureUtility and initializing frame reader.");
            _MediaCaptureUtility = new MediaCaptureUtility();
            await _MediaCaptureUtility.InitializeMediaFrameReaderAsync(MediaCaptureProfiles);
            StatusBlock.text = $"Camera started. Running!";
            Debug.Log("Successfully initialized frame reader.");
        }
        catch (Exception ex)
        {
            StatusBlock.text = $"Failed to start camera: {ex.Message}. Using loaded/picked image.";
        }

        // Get the unity spatial coordinate system
        try
        {
            _unityCoordinateSystem = 
                Marshal.GetObjectForIUnknown(WorldManager.GetNativeISpatialCoordinateSystemPtr()) as SpatialCoordinateSystem;
            StatusBlock.text = $"Acquired unity coordinate system!";
            Debug.Log("Successfully cached pointer to Unity spatial coordinate system.");
        }
        catch (Exception ex)
        {
            StatusBlock.text = $"Failed to get Unity spatial coordinate system: {ex.Message}.";
        }
#endif
    }


#if ENABLE_WINMD_SUPPORT
    /// <summary>
    /// Method to extract important paramters and software bitmap from
    /// media frame reference and send to opencv dll for marker-based or
    /// board-based tracking.
    /// </summary>
    private void HandleArUcoTracking(Windows.Media.Capture.Frames.MediaFrameReference mediaFrameReference)
    {
        // Request software bitmap from media frame reference
        var softwareBitmap = mediaFrameReference?.VideoMediaFrame?.SoftwareBitmap;
        Debug.Log("Successfully requested software bitmap.");

        if (softwareBitmap != null)
        {
            // Cache the current camera projection transform (not using currently)
            var cameraProjectionTransform = mediaFrameReference.VideoMediaFrame.CameraIntrinsics.UndistortedProjectionTransform;
            Debug.Log($"_cameraProjectionTransform: {cameraProjectionTransform}");

            // Cache the current camera intrinsics
            OpenCVRuntimeComponent.CameraCalibrationParams calibParams = 
                new OpenCVRuntimeComponent.CameraCalibrationParams(System.Numerics.Vector2.Zero, System.Numerics.Vector2.Zero, System.Numerics.Vector3.Zero, System.Numerics.Vector2.Zero, 0, 0);

            switch (CalibrationParameterType)
            {
                // Cache from user-defined parameters 
                case ArUcoUtils.CameraCalibrationParameterType.UserDefined:
                    calibParams = new OpenCVRuntimeComponent.CameraCalibrationParams(
                        new System.Numerics.Vector2(UserDefinedCalibParams.focalLength.x, UserDefinedCalibParams.focalLength.y), // Focal length
                        new System.Numerics.Vector2(UserDefinedCalibParams.principalPoint.x, UserDefinedCalibParams.principalPoint.y), // Principal point
                        new System.Numerics.Vector3(UserDefinedCalibParams.radialDistortion.x, UserDefinedCalibParams.radialDistortion.y, UserDefinedCalibParams.radialDistortion.z), // Radial distortion
                        new System.Numerics.Vector2(UserDefinedCalibParams.tangentialDistortion.x, UserDefinedCalibParams.tangentialDistortion.y), // Tangential distortion
                        (int)mediaFrameReference.VideoMediaFrame.CameraIntrinsics.ImageWidth, // Image width
                        (int)mediaFrameReference.VideoMediaFrame.CameraIntrinsics.ImageHeight); // Image height
                        Debug.Log($"User-defined calibParams: [{calibParams}]");
                    break;

                // Cache from the video media frame
                case ArUcoUtils.CameraCalibrationParameterType.PerFrame:
                    calibParams = new OpenCVRuntimeComponent.CameraCalibrationParams(
                        mediaFrameReference.VideoMediaFrame.CameraIntrinsics.FocalLength, // Focal length
                        mediaFrameReference.VideoMediaFrame.CameraIntrinsics.PrincipalPoint, // Principal point
                        mediaFrameReference.VideoMediaFrame.CameraIntrinsics.RadialDistortion, // Radial distortion
                        mediaFrameReference.VideoMediaFrame.CameraIntrinsics.TangentialDistortion, // Tangential distortion
                        (int)mediaFrameReference.VideoMediaFrame.CameraIntrinsics.ImageWidth, // Image width
                        (int)mediaFrameReference.VideoMediaFrame.CameraIntrinsics.ImageHeight); // Image height
                    Debug.Log($"Per-frame calibParams: [{calibParams}]");

                    break;
                default:
                    break;
            }

            // Cache the current camera frame coordinate system
            _frameCoordinateSystem = mediaFrameReference.CoordinateSystem;
            Debug.Log($"_frameCoordinateSystem set from media frame reference");

            switch (ArUcoTrackingType)
            {
                case ArUcoUtils.ArUcoTrackingType.Markers:
                    DetectMarkers(softwareBitmap, calibParams);
                    break;

                case ArUcoUtils.ArUcoTrackingType.CustomBoard:
                    DetectBoard(softwareBitmap, calibParams);
                    break;

                case ArUcoUtils.ArUcoTrackingType.None:
                    StatusBlock.text = $"Not running tracking...";
                    break;

                default:
                    StatusBlock.text = $"No option selected for tracking...";
                    break;
            }
        }
        // Dispose of the bitmap
        softwareBitmap?.Dispose();
    }
#endif

    /// <summary>
    /// Handle initialization of local scale and display of 
    /// marker and board gameobjects.
    /// </summary>
    private void HandleGoIntitialization()
    {
        // Set the scale of markers for visualization 
        TrackingGos.MarkerGoLeftEye.transform.localScale = new Vector3(ArUcoBoardPositions.markerSizeForSingle, ArUcoBoardPositions.markerSizeForSingle, ArUcoBoardPositions.markerSizeForSingle);
        TrackingGos.MarkerGoRightEye.transform.localScale = new Vector3(ArUcoBoardPositions.markerSizeForSingle, ArUcoBoardPositions.markerSizeForSingle, ArUcoBoardPositions.markerSizeForSingle);

        switch (ArUcoTrackingType)
        {
            case ArUcoUtils.ArUcoTrackingType.Markers:
                switch (HMDCalibrationType)
                {
                    case ArUcoUtils.HMDCalibrationType.UserDefined:
                        // Make the markers visible in the scene
                        TrackingGos.MarkerGoLeftEye.SetActive(true);
                        TrackingGos.MarkerGoRightEye.SetActive(true);
                        Debug.Log("Preparing for marker tracking with user defined transform.");
                        break;
                    case ArUcoUtils.HMDCalibrationType.PointBased:
                        switch (_HMDCalibrationStatus)
                        {
                            case ArUcoUtils.HMDCalibrationStatus.NotCalibrating:
                                Debug.Log("Not calibrating...");
                                break;
                            
                            case ArUcoUtils.HMDCalibrationStatus.StartedCalibration:
                                // Hide the markers in the scene
                                TrackingGos.MarkerGoLeftEye.SetActive(false);
                                TrackingGos.MarkerGoRightEye.SetActive(false);
                                Debug.Log("Computing transform to use in marker tracking.");
                                break;
                            
                            case ArUcoUtils.HMDCalibrationStatus.CompletedCalibration:
                                // After calibration, show the markers in the scene
                                TrackingGos.MarkerGoLeftEye.SetActive(true);
                                TrackingGos.MarkerGoRightEye.SetActive(true);
                                Debug.Log("Computed transform, make markers visible in marker tracking.");
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }

                break;
            case ArUcoUtils.ArUcoTrackingType.CustomBoard:
                TrackingGos.SetGameObjectsFromRng();

                // Make the markers visible in the scene
                TrackingGos.BoardGoLeftEye.SetActive(true);
                TrackingGos.BoardGoRightEye.SetActive(true);
                Debug.Log("Preparing for board tracking.");

                break;
            case ArUcoUtils.ArUcoTrackingType.None:
                Debug.Log("No tracking type selected.");
                break;
            default:
                break;
        }
    }

#if ENABLE_WINMD_SUPPORT
    private void DetectMarkers(SoftwareBitmap softwareBitmap, OpenCVRuntimeComponent.CameraCalibrationParams calibParams)
    {
        UnityEngine.WSA.Application.InvokeOnAppThread(() =>
        {
            // Remove world anchor from game object
            if (_isWorldAnchored)
            {
                DestroyImmediate(TrackingGos.MarkerGoLeftEye.GetComponent<WorldAnchor>());
                DestroyImmediate(TrackingGos.MarkerGoRightEye.GetComponent<WorldAnchor>());
                _isWorldAnchored = false;
                
                Debug.Log("DetectMarkers: removed current world anchor.");
            }
        }, false);


        // Get marker detections from opencv component
        var markers = CvUtils.DetectMarkers(softwareBitmap, calibParams);

        if (markers.Count != 0)
        {
            // Iterate across detections
            foreach (var marker in markers)
            {
                switch (_HMDCalibrationStatus)
                {
                    case ArUcoUtils.HMDCalibrationStatus.NotCalibrating:
                        TransformUnityCamera = ArUcoUtils.GetTransformInUnityCamera(
                            ArUcoUtils.Vec3FromFloat3(marker.Position),
                            ArUcoUtils.RotationQuatFromRodrigues(ArUcoUtils.Vec3FromFloat3(marker.Rotation)));
                        break;
                    
                    case ArUcoUtils.HMDCalibrationStatus.StartedCalibration:
                        // Get the average transform in unity camera space
                        TransformUnityCamera = GetAverageTransform(
                            ArUcoUtils.Vec3FromFloat3(marker.Position),
                            ArUcoUtils.RotationQuatFromRodrigues(ArUcoUtils.Vec3FromFloat3(marker.Rotation)),
                            NumMovingAvgPts);
                        break;
                    
                    case ArUcoUtils.HMDCalibrationStatus.CompletedCalibration:
                        TransformUnityCamera = ArUcoUtils.GetTransformInUnityCamera(
                            ArUcoUtils.Vec3FromFloat3(marker.Position),
                            ArUcoUtils.RotationQuatFromRodrigues(ArUcoUtils.Vec3FromFloat3(marker.Rotation)));
                        break;
                }
                Debug.Log($"transformUnityCamera: {TransformUnityCamera}");

                // Camera view transform used for transform chain
                CameraToWorldUnity = GetViewToUnityTransform(_frameCoordinateSystem);
                Debug.Log($"c2w_unity: {CameraToWorldUnity}");

                // Right and left eye transforms, apply user defined transform in chain
                var transformUnityWorldLeft = CameraToWorldUnity * PredefinedTransform.UserDefinedTransformLeftEye * TransformUnityCamera;
                var transformUnityWorldRight = CameraToWorldUnity * PredefinedTransform.UserDefinedTransformRightEye * TransformUnityCamera;

                // Update the UI with result
                UnityEngine.WSA.Application.InvokeOnAppThread(() =>
                {
                    StatusBlock.text = $"Detected: {markers.Count} markers";

                    // Left eye
                    TrackingGos.MarkerGoLeftEye.transform.SetPositionAndRotation(
                        ArUcoUtils.GetVectorFromMatrix(transformUnityWorldLeft),
                        ArUcoUtils.GetQuatFromMatrix(transformUnityWorldLeft));

                    // Right eye
                    TrackingGos.MarkerGoRightEye.transform.SetPositionAndRotation(
                        ArUcoUtils.GetVectorFromMatrix(transformUnityWorldRight),
                        ArUcoUtils.GetQuatFromMatrix(transformUnityWorldRight));

                }, false);
            }
        }

        // If no markers in scene, anchor marker to last position of game object
        // if we have not already anchored the game objects
        else 
        {
            UnityEngine.WSA.Application.InvokeOnAppThread(() =>
            {
                StatusBlock.text = $"No markers detected";

                // Add a world anchor to the attached gameobject
                TrackingGos.MarkerGoLeftEye.AddComponent<WorldAnchor>();
                TrackingGos.MarkerGoRightEye.AddComponent<WorldAnchor>();
                _isWorldAnchored = true;

            }, false);
            
            Debug.Log("DetectMarkers: updated world anchor position.");
        }
    }

    private void DetectBoard(SoftwareBitmap softwareBitmap, OpenCVRuntimeComponent.CameraCalibrationParams calibParams)
    {
        UnityEngine.WSA.Application.InvokeOnAppThread(() =>
        {
            // Remove world anchor from game object
            if (_isWorldAnchored)
            {
                DestroyImmediate(TrackingGos.BoardGoLeftEye.GetComponent<WorldAnchor>());
                DestroyImmediate(TrackingGos.BoardGoRightEye.GetComponent<WorldAnchor>());
                _isWorldAnchored = false;
            }

        }, false);

        // Get marker detections from opencv component
        var board = CvUtils.DetectBoard(softwareBitmap, calibParams);

        if (board.IsDetected)
        {
            // Get the transform from C++ component and format for Unity coordinate system
            var transformUnityCamera = ArUcoUtils.GetTransformInUnityCamera(
                ArUcoUtils.Vec3FromFloat3(board.Position),
                ArUcoUtils.RotationQuatFromRodrigues(ArUcoUtils.Vec3FromFloat3(board.Rotation)));
            Debug.Log($"transformUnityCamera: {transformUnityCamera}");

            // Camera view transform used for transform chain
            var c2w_unity = GetViewToUnityTransform(_frameCoordinateSystem);
            Debug.Log($"c2w_unity: {c2w_unity}");

            // Right and left eye transforms, apply user defined transform in chain
            var transformUnityWorldLeft = c2w_unity * PredefinedTransform.UserDefinedTransformLeftEye * transformUnityCamera;
            var transformUnityWorldRight = c2w_unity * PredefinedTransform.UserDefinedTransformRightEye * transformUnityCamera;

            // Update the UI with result
            UnityEngine.WSA.Application.InvokeOnAppThread(() =>
            {
                StatusBlock.text = $"Detected: aruco board";

                // Left eye
                TrackingGos.BoardGoLeftEye.transform.SetPositionAndRotation(
                    ArUcoUtils.GetVectorFromMatrix(transformUnityWorldLeft),
                    ArUcoUtils.GetQuatFromMatrix(transformUnityWorldLeft));

                // Right eye
                TrackingGos.BoardGoRightEye.transform.SetPositionAndRotation(
                    ArUcoUtils.GetVectorFromMatrix(transformUnityWorldRight),
                    ArUcoUtils.GetQuatFromMatrix(transformUnityWorldRight));
            }, false);
        }

        // If no markers in scene, anchor marker to last position of game object
        // if we have not already anchored the game objects
        else
        {
            UnityEngine.WSA.Application.InvokeOnAppThread(() =>
            {
                StatusBlock.text = $"No board detected";

                // Add a world anchor to the attached gameobject
                TrackingGos.BoardGoLeftEye.AddComponent<WorldAnchor>();
                TrackingGos.BoardGoRightEye.AddComponent<WorldAnchor>();
                _isWorldAnchored = true;

            }, false);

            Debug.Log("DetectMarkers: updated world anchor position.");
        }
    }
#endif

#if ENABLE_WINMD_SUPPORT
    //https://github.com/microsoft/MixedReality-SpectatorView/blob/7796da6acb0ae41bed1b9e0e9d1c5c683b4b8374/src/SpectatorView.Unity/Assets/PhotoCapture/Scripts/HoloLensCamera.cs#L1256
    /// <summary>
    /// Create the camera extrinsics from unity coordinate system
    /// and the current frame coordinate system.
    /// </summary>
    /// <param name="frameCoordinateSystem"></param>
    /// <returns></returns>
    private Matrix4x4 GetViewToUnityTransform(
        SpatialCoordinateSystem frameCoordinateSystem)
    {
        if (frameCoordinateSystem == null || _unityCoordinateSystem == null)
        {
            return Matrix4x4.identity;
        }

        // Get the reference transform from camera frame to unity space
        System.Numerics.Matrix4x4? cameraToUnityRef = frameCoordinateSystem.TryGetTransformTo(_unityCoordinateSystem);
        
        // Return identity if value does not exist
        if (!cameraToUnityRef.HasValue)
            return Matrix4x4.identity;

        // No cameraViewTransform availabnle currently, using identity for HL2
        // Inverse of identity is identity
        var viewToCamera = Matrix4x4.identity;
        var cameraToUnity = ArUcoUtils.Mat4x4FromFloat4x4(cameraToUnityRef.Value);

        // Compute transform to relate winrt coordinate system with unity coordinate frame (viewToUnity)
        // WinRT transfrom -> Unity transform by transpose and flip row 3
        var viewToUnityWinRT = viewToCamera * cameraToUnity;
        var viewToUnity = Matrix4x4.Transpose(viewToUnityWinRT);
        viewToUnity.m20 *= -1.0f;
        viewToUnity.m21 *= -1.0f;
        viewToUnity.m22 *= -1.0f;
        viewToUnity.m23 *= -1.0f;

        return viewToUnity;
    }

    /// <summary>
    /// Method to control the collection of points. Called by gesture
    /// handler or keyboard input.
    /// </summary>
    private void ControlCalib()
    {
        switch (HMDCalibrationType)
        {
            // If we are using a predefined transform hide the calibration 
            case ArUcoUtils.HMDCalibrationType.UserDefined:
                CollectPointCorrespondences.HideCalibrationReticle();
                break;

            case ArUcoUtils.HMDCalibrationType.PointBased:
                switch (_HMDCalibrationStatus)
                {
                    // If we are using a predefined transform hide the calibration 
                    case ArUcoUtils.HMDCalibrationStatus.NotCalibrating:
                        CollectPointCorrespondences.HideCalibrationReticle();
                        break;

                    // We have started calibrating the HMD
                    case ArUcoUtils.HMDCalibrationStatus.StartedCalibration:
#if ENABLE_WINMD_SUPPORT
                        var isDoneCalibrating = CollectPointCorrespondences.CollectPointCorrespondence(
                            TransformUnityCamera,
                            CameraToWorldUnity,
                            CvUtils);
#endif
                        // If we are completed calibration, change enum status and hide reticle
                        if (isDoneCalibrating)
                        {
                            _HMDCalibrationStatus = ArUcoUtils.HMDCalibrationStatus.CompletedCalibration;
                            CollectPointCorrespondences.HideCalibrationReticle();
                            ControlCalib();
                        }
                        break;

                    // We have completed calibration, reset the aruco tracking and 
                    // begin board tracking with the new predefined transform
                    case ArUcoUtils.HMDCalibrationStatus.CompletedCalibration:

                        // Set the user-defined transform for each eye to be the newly computed transform
                        PredefinedTransform.UserDefinedTransformLeftEye = CollectPointCorrespondences.CalibMatrixLeft;
                        PredefinedTransform.UserDefinedTransformRightEye = CollectPointCorrespondences.CalibMatrixRight;
                        CollectPointCorrespondences.HideCalibrationReticle();
                        
                        // Stop marker tracking, restart with new tracking type and using 
                        // computed transform offset 
                        _isRunning = false;
                        Start();

                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
#endif

    /// <summary>
    /// Get the average transform of the head relative camera points
    /// Exclude zero values from point correspondence holder.
    /// </summary>
    /// <param name="t"></param>
    /// <param name="r"></param>
    /// <returns></returns>
    Matrix4x4 GetAverageTransform(
        Vector3 t,
        Quaternion r,
        int numMovingAvgPts)
    {
        // Carry an average transform value for head-relative point locations
        // If we've reached queue length, remove a point
        if (_posCamQ.Count == numMovingAvgPts)
        {
            // Dequeue element
            _ = _posCamQ.Dequeue();
            _ = _rotCamQ.Dequeue();
        }

        // Enqueue the current point
        _posCamQ.Enqueue(t);
        _rotCamQ.Enqueue(r);

        // Get average of this array
        var avgP = ArUcoUtils.ArrayAvg(_posCamQ.ToArray());

        // Get average of rotations
        var avgR = ArUcoUtils.CalcAverageQuaternion(_rotCamQ.ToArray());

        // Return the transform in unity space
        return ArUcoUtils.GetTransformInUnityCamera(avgP, avgR);
    }
}