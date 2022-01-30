function sim = compute_similarity(im_bw, im_gt_bw, mask, min_xy, max_xy)
%compute_similarity Compute the DICE similarity metric between input
%images and a defined region of interest.
% https://www.mathworks.com/help/images/ref/dice.html

figure; 
imshowpair(im_bw, im_gt_bw, 'montage');
title('Trace and truth grayscale');

% Show the images
figure; 
imshowpair(im_bw(min_xy(2): max_xy(2), min_xy(1): max_xy(1)), im_gt_bw(min_xy(2): max_xy(2), min_xy(1): max_xy(1)), 'montage')
title('Trace and truth grayscale masked cropped');

% Apply mask to images
im_bw(mask == 0) = 255;
im_gt_bw(mask == 0) = 255;

% Crop regions to reduce processing time
im_c = im_bw(min_xy(2): max_xy(2), min_xy(1): max_xy(1));
im_gt_c = im_gt_bw(min_xy(2): max_xy(2), min_xy(1): max_xy(1));

% Binarize the images for DICE metrics
% Fill holes in images
% https://www.mathworks.com/matlabcentral/answers/273022-how-to-fill-the-region-inside-a-contour
se = strel('disk', 4, 0);
im_bin = imbinarize(im_c);
im_bin = imclose(~im_bin, se);
im_bin = imfill(im_bin, 'holes'); 

im_gt_bin = imbinarize(im_gt_c);
im_gt_bin = imclose(~im_gt_bin, se);
im_gt_bin = imfill(im_gt_bin, 'holes');

% Show the images
figure; 
imshowpair(im_bin, im_gt_bin, 'montage')
title('Trace and truth grayscale mask and filled binarized mask');

% Calculate dice index and show pairs
sim = dice(im_gt_bin, im_bin);

figure;
imshowpair(im_gt_bin, im_bin);
title(['Dice Index = ' num2str(sim)]);

end

