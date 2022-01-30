using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PredefinedTransform : MonoBehaviour
{
    // Apply a user defined transform for the alignment task
    public Matrix4x4 UserDefinedTransformRightEye = Matrix4x4.identity;
    public Matrix4x4 UserDefinedTransformLeftEye = Matrix4x4.identity;
}
