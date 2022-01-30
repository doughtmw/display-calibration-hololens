using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

#if ENABLE_WINMD_SUPPORT
using Windows.UI.Xaml;
using Windows.Graphics.Imaging;
using Windows.Perception.Spatial;
#endif

using UnityEngine;
using UnityEngine.UI;
using UnityEngine.XR.WSA;
using UnityEngine.XR.WSA.Input;
using System.Threading;

// Class to store point correspondence information for
// each individual eye during calibration
namespace ArUcoDetectionHoloLensUnity
{
    public class PointCorrespondencesHolder : MonoBehaviour
    {
        // Public variables
        public string eye;
        public Camera.StereoscopicEye stereoEye;
        public Camera cam;
        public Canvas canvas;
        public Text txt;
        public GameObject reticleGo;

        // Class instance of the reticle point locations for 
        // storing shared target locations
        public ReticlePointLocations ReticlePointLocations;

        /// <summary>
        /// Private variables to hold point correspondences from head-relative
        /// camera points and head-relative marker points
        /// </summary>
        private List<string> _headRelativeCameraPoints3D;
        private List<string> _headRelativeMarkerPoints3D;
        private IList<System.Numerics.Vector3> _headRelativeCameraPointVector3D;
        private IList<System.Numerics.Vector3> _headRelativeMarkerPointVector3D;
        private int _globalPointCount = 0;

        // Encapsulated fields for storing spaam 
        // calibration transform matrices
        private Matrix4x4 _headRelativeCameraPoint3DToMarkerPoint3D;
        public Matrix4x4 Hrcp3DToHrmp3D
        {
            get { return _headRelativeCameraPoint3DToMarkerPoint3D; }
        }

        // Start is called before the first frame update
        public void Initialize()
        {
            // Initialize lists for holding point correspondences
            _headRelativeCameraPoints3D = new List<string>();
            _headRelativeMarkerPoints3D = new List<string>();
            _headRelativeCameraPointVector3D = new List<System.Numerics.Vector3>();
            _headRelativeMarkerPointVector3D = new List<System.Numerics.Vector3>();

            // Reset calibration point count
            _globalPointCount = 0;

            txt.text = "Double tap or press spacebar to begin collecting points.";
        }

        /// <summary>
        /// Save the calibration point locations to a text file
        /// for inspection.
        /// </summary>
        /// <param name="transformUnityCamera"></param>
        /// <param name="calibPointLocations"></param>
        /// <returns></returns>
#if ENABLE_WINMD_SUPPORT
        public bool SaveCalibrationPointCorrespondences(
            Matrix4x4 transformUnityCamera,
            Matrix4x4 cameraToWorldUnity,
            OpenCVRuntimeComponent.CvUtils cvUtils)
        {
            // Increment calibration point count
            _globalPointCount += 1;
            Debug.LogFormat("ArUcoMarkerDetection: added point to list. Count: {0}",
                _globalPointCount);

            // Get world pose of marker game objects in frame
            var goPosWorldMarkerPoint3D = reticleGo.transform.position;
            var goRotWorldMarkerPoint3D = reticleGo.transform.rotation;

            // Compose a matrix from pose
            var goTransformWorldMarkerPoint3D = Matrix4x4.TRS(
                goPosWorldMarkerPoint3D,
                goRotWorldMarkerPoint3D,
                Vector3.one);

            // HeadRelativeCamera = WorldToCamera * WorldMarkerPoint
            var goTransformHeadRelativeCameraMarkerPoint3D = cameraToWorldUnity.inverse * goTransformWorldMarkerPoint3D;

            // Get transform from matrix
            var goTransformHeadRelativeMarkerPoint3D = ArUcoUtils.GetVectorFromMatrix(goTransformHeadRelativeCameraMarkerPoint3D);

            Debug.LogFormat("Reticle head-relative camera point 3D: {0}, {1}, {2}.",
                goTransformHeadRelativeMarkerPoint3D.x,
                goTransformHeadRelativeMarkerPoint3D.y,
                goTransformHeadRelativeMarkerPoint3D.z);

            // Current 3D head-relative camera postition of tracked object 
            var goTransformHeadRelativeCameraPoint3D = ArUcoUtils.GetVectorFromMatrix(transformUnityCamera);
            Debug.LogFormat("Marker head-relative camera point 3D: {0}, {1}, {2}.",
                goTransformHeadRelativeCameraPoint3D.x,
                goTransformHeadRelativeCameraPoint3D.y,
                goTransformHeadRelativeCameraPoint3D.z);

            // Cache the current head relative marker point of the game objects
            // and write to text file for debugging purposes
            _headRelativeMarkerPointVector3D.Add(
                new System.Numerics.Vector3(
                    goTransformHeadRelativeMarkerPoint3D.x,
                    goTransformHeadRelativeMarkerPoint3D.y,
                    goTransformHeadRelativeMarkerPoint3D.z));

            _headRelativeMarkerPoints3D.Add(
                goTransformHeadRelativeMarkerPoint3D.x.ToString() + "," +
                goTransformHeadRelativeMarkerPoint3D.y.ToString() + "," +
                goTransformHeadRelativeMarkerPoint3D.z.ToString());
            ArUcoUtils.WriteToText("_headRelativeMarkerPoints3D" + eye + ".txt", _headRelativeMarkerPoints3D);

            // Cache the current head relative camera point of the game objects and write to 
            // text file for debugging 
            _headRelativeCameraPointVector3D.Add(
                new System.Numerics.Vector3(
                    goTransformHeadRelativeCameraPoint3D.x,
                    goTransformHeadRelativeCameraPoint3D.y,
                    goTransformHeadRelativeCameraPoint3D.z));

            _headRelativeCameraPoints3D.Add(
                goTransformHeadRelativeCameraPoint3D.x.ToString() + "," +
                goTransformHeadRelativeCameraPoint3D.y.ToString() + "," +
                goTransformHeadRelativeCameraPoint3D.z.ToString());
            ArUcoUtils.WriteToText("_headRelativeCameraPoints3D" + eye + ".txt", _headRelativeCameraPoints3D);

            // Debug text field
            txt.text = $"Collecting points... Count: {_globalPointCount}";

            // Move calibration reticle to new location and update text
            // Set the local position and rotation of the reticle game object
            if (_globalPointCount < ReticlePointLocations.calibPointLocations.Count)
            {
                reticleGo.transform.localPosition = ReticlePointLocations.calibPointLocations[_globalPointCount];
                reticleGo.transform.localEulerAngles = ReticlePointLocations.calibPointEulerRotations[_globalPointCount];

                Debug.LogFormat("Moved reticle gameobject to new position");
                return false;
            }
            // If we have completed the calibration procedure
            else if (_globalPointCount == ReticlePointLocations.calibPointLocations.Count)
            {
                // Send these points to C++ WinRT plugin and get (3D-3D) rigid transform to 
                // minimize the euclidean distance between point sets
                // if we have reached the termination criteria for number of points
#if ENABLE_WINMD_SUPPORT
                var hrcp3DTohrmp3D = cvUtils.RigidTransform3D3D(
                    _headRelativeCameraPointVector3D,
                    _headRelativeMarkerPointVector3D);

                // Cache the value for returning to aruco script
                // and modifying the displayed content
                _headRelativeCameraPoint3DToMarkerPoint3D = ArUcoUtils.Mat4x4FromFloat4x4(hrcp3DTohrmp3D);

                // Debug to console window
                ArUcoUtils.DebugMatrix(_headRelativeCameraPoint3DToMarkerPoint3D, "hrcp3DTohrmp3DSpaam" + eye);

                // Debug the intrinsic view and projection matrices (not used currently)
                var eyeViewMat = cam.GetStereoViewMatrix(stereoEye);
                ArUcoUtils.DebugMatrix(eyeViewMat, "StereoViewMatrix" + eye);

                var eyeProjMat = cam.GetStereoProjectionMatrix(stereoEye);
                ArUcoUtils.DebugMatrix(eyeProjMat, "StereoProjectionMatrix" + eye);
#endif
                _globalPointCount = 0;
                Debug.Log("ArUcoMarkerDetection: finished calibration procedure...");

                return true;
            }
            else
            {
                Debug.Log("ArUcoMarkerDetection: done calibrating...");
                return true;
            }
        }
#endif
    }
}
