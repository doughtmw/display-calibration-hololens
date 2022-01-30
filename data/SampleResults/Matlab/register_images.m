function [im_reg_bw, im_gt_bw] = register_images(im, im_gt)

% Convert format from uint8 to double
im  = im2double(im);
im_gt = im2double(im_gt);

% Convert to gray scale
if (size(im, 3) == 3)
    im_bw = rgb2gray(im);
else
    im_bw = im;  
end
if (size(im_gt, 3) == 3)
    im_gt_bw = rgb2gray(im_gt);
else
    im_gt_bw = im_gt;
end
    
figure; 
imshowpair(im_bw, im_gt_bw, 'montage')
title('Trace and truth grayscale prior to registration');

% Align the trace with the ground truth
% https://www.mathworks.com/help/images/ref/imregister.html
% movingRegistered = imregister(moving, fixed, 'affine', optimizer, metric);
% [optimizer, metric] = imregconfig('monomodal');
[optimizer, metric] = imregconfig('multimodal');

im_reg_bw = imregister(im_bw, im_gt_bw, 'rigid', optimizer, metric);

figure; 
imshowpair(im_reg_bw, im_gt_bw, 'montage')
title('Trace and truth grayscale after registration');

end