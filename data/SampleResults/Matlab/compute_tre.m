function tre = compute_tre(im_bw, im_gt_bw, mask, min_xy, max_xy, trace_low, trace_high, gt_low, gt_high)

figure; 
imshowpair(im_bw, im_gt_bw, 'montage');
title('Trace and truth grayscale');

% Apply mask to images
im_bw(mask == 0) = 255;
im_gt_bw(mask == 0) = 255;

% Crop regions to reduce processing time
im_c = im_bw(min_xy(2): max_xy(2), min_xy(1): max_xy(1));
im_gt_c = im_gt_bw(min_xy(2): max_xy(2), min_xy(1): max_xy(1));

% Show the images
figure; 
imshowpair(im_c, im_gt_c, 'montage')
title('Trace and truth grayscale masked cropped');

% Binarize the images for DICE metrics
% Fill holes in images
% https://www.mathworks.com/matlabcentral/answers/273022-how-to-fill-the-region-inside-a-contour
se = strel('disk', 1, 0);
im_bin = imbinarize(im_c);
im_bin = imclose(~im_bin, se);
im_bin = imfill(im_bin, 'holes'); 

im_gt_bin = imbinarize(im_gt_c);
im_gt_bin = imclose(~im_gt_bin, se);
im_gt_bin = imfill(im_gt_bin, 'holes');

% Manually fine-tune the ROI if there are any extra points in data
im_bin_fine_tine = roipoly(im_bin);
im_gt_bin_fine_tine = roipoly(im_gt_bin);

% Apply the ROIs
im_bin = im_bin .* im_bin_fine_tine;
im_gt_bin = im_gt_bin .* im_gt_bin_fine_tine;

% Show the images
figure; 
imshowpair(im_bin, im_gt_bin, 'montage')
title('Trace and truth grayscale mask and filled binarized mask');

% Show the images
figure; 
imshow(im_bin)
title('Trace filled binarized mask with circles identified');

% Trace
% Fit circles in the images, may have to tweak
[centers, radii, metric] = imfindcircles(im_bin, [trace_low trace_high], 'ObjectPolarity','bright','Sensitivity',0.9987);

% Filter overlapping circles
[centers, radii, metric] = filter_circles(centers, radii, metric);

% Get top 19 circles which do not overlap
centersStrong19 = centers(1:19,:); 
radiiStrong19 = radii(1:19);
metricStrong19 = metric(1:19);
viscircles(centersStrong19, radiiStrong19,'EdgeColor','b');

% Show the images
figure; 
imshow(im_gt_bin)
title('Truth filled binarized mask with circles identified');

% Truth
% Fit circles in the images, may have to tweak
[centers_gt, radii_gt, metric_gt] = imfindcircles(im_gt_bin, [gt_low gt_high], 'ObjectPolarity','bright','Sensitivity',0.9987);

% Filter overlapping circles
[centers_gt, radii_gt, metric_gt] = filter_circles(centers_gt, radii_gt, metric_gt);

% Get top 19 circles which do not overlap
centersStrong19_gt = centers_gt(1:19,:); 
radiiStrong19_gt = radii_gt(1:19);
metricStrong19_gt = metric_gt(1:19);
viscircles(centersStrong19_gt, radiiStrong19_gt,'EdgeColor','r');

% knn for finding nearest neighbours in images (for point correspondences)
Idx = knnsearch(centersStrong19, centersStrong19_gt);

% Get difference in each closest point correspondence
diff = centersStrong19(Idx,:) - centersStrong19_gt;

% Compute the tre for each point correspondence
tre = sqrt((diff(:,1)).^2 + (diff(:,2)).^2);

end