using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

public class TrackingGos : MonoBehaviour
{
    /// <summary>
    /// Game objects for use in tracking. List is set so that 
    /// we can randomly select a board for tracking
    /// </summary>
    public GameObject MarkerGoRightEye;
    public GameObject MarkerGoLeftEye;
    
    public List<GameObject> BoardGoListRightEye;
    public List<GameObject> BoardGoListLeftEye;
    public GameObject BoardGoRightEye { get; set; }
    public GameObject BoardGoLeftEye { get; set; }

    public void SetGameObjectsFromRng()
    {
        // Set the board go to use from rng
        // Create the RNG for selecting the current tracing target
        // between [min] and [max] exclusive
        int rng = UnityEngine.Random.Range(0, BoardGoListLeftEye.Count - 1);
        BoardGoRightEye = BoardGoListRightEye.ElementAt(rng);
        BoardGoLeftEye = BoardGoListLeftEye.ElementAt(rng);
    }
}
