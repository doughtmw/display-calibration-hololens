using System.Collections;
using System.Collections.Generic;
using UnityEngine;


namespace ArUcoDetectionHoloLensUnity
{
    public class CollectPointCorrespondences : MonoBehaviour
    {
        // Public variables
        public PointCorrespondencesHolder ptsRight;
        public PointCorrespondencesHolder ptsLeft;

        // Private variables
        private bool _isCalibrationCompletedRightEye = false;
        private bool _isCalibrationCompletedLeftEye = false;
        private bool _isRightEyeInit = false;
        private bool _isLeftEyeInit = false;

        /// <summary>
        /// Encapsulated fields for storing calibration matrices
        /// from spaam. Cache the headRelativeCameraPoint3D to MarkerPoint3D transform.
        /// </summary>
        public Matrix4x4 CalibMatrixRight { get; set; }
        public Matrix4x4 CalibMatrixLeft { get; set; }

        /// <summary>
        /// Initialize the point correspondence collector.
        /// </summary>
        public void Initialize()
        {
            // Begin with the right eye.
            ptsRight.canvas.gameObject.SetActive(true);
            ptsLeft.canvas.gameObject.SetActive(false);
        }

#if ENABLE_WINMD_SUPPORT
        /// <summary>
        /// Collect point correspondences for the spaam 
        /// calibration procedure. Take UnityCameraTransform and 
        /// cameraToWorldUnityTransform as input
        /// </summary>
        /// <param name="transformUnityCamera"></param>
        public bool CollectPointCorrespondence(
            Matrix4x4 transformUnityCamera,
            Matrix4x4 cameraToWorldUnity,
            OpenCVRuntimeComponent.CvUtils cvUtils)
        {
            // If right eye is not initialized, initialize the lists for storing point locations
            if (!_isRightEyeInit)
            {
                ptsRight.Initialize();
                _isRightEyeInit = true;
            }
            // If we have not yet calibrated the right eye
            // enable the correct camera configuration and save the
            // point correspondences
            if (!_isCalibrationCompletedRightEye && !_isCalibrationCompletedLeftEye)
            {
                // Save point correspondence, returns false until completed
                _isCalibrationCompletedRightEye =
                    ptsRight.SaveCalibrationPointCorrespondences(
                    transformUnityCamera,
                    cameraToWorldUnity,
                    cvUtils);

                // When completed, switch to next eye
                if (_isCalibrationCompletedRightEye)
                {
                    // Hide right canvas, show left canvas
                    ptsRight.canvas.gameObject.SetActive(false);
                    ptsLeft.canvas.gameObject.SetActive(true);

                    // Cache the computed transform for the right eye
                    CalibMatrixRight = ptsRight.Hrcp3DToHrmp3D;
                    Debug.Log("Finished calibrating right eye.");
                }
                // Still calibrating, return false
                return false;
            }

            // left eye
            else if (_isCalibrationCompletedRightEye && !_isCalibrationCompletedLeftEye)
            {
                // Initialize the lists for storing point locations
                if (!_isLeftEyeInit)
                {
                    ptsLeft.Initialize();
                    _isLeftEyeInit = true;
                }

                // Save point correspondence, returns false until completed
                _isCalibrationCompletedLeftEye =
                    ptsLeft.SaveCalibrationPointCorrespondences(
                    transformUnityCamera,
                    cameraToWorldUnity,
                    cvUtils);

                // Hide the canvas for both eyes
                if (_isCalibrationCompletedRightEye && _isCalibrationCompletedLeftEye)
                {
                    ptsRight.canvas.gameObject.SetActive(false);
                    ptsLeft.canvas.gameObject.SetActive(false);

                    // Cache the computed transform for the left eye
                    CalibMatrixLeft = ptsLeft.Hrcp3DToHrmp3D;
                    Debug.Log("Finished calibrating left eye.");
                    
                    // Done calibrating
                    return true;
                }

                // Still calibrating, return false
                return false;
            }

            // Done calibrating
            else
            {
                // Return true when done calibrating 
                return true;
            }
        }
#endif

        // For use when we are using predefined 
        // calibration matrices for testing
        public void HideCalibrationReticle()
        {
            ptsRight.canvas.gameObject.SetActive(false);
            ptsLeft.canvas.gameObject.SetActive(false);
        }
    }
}