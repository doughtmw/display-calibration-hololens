% Average symmetric surface distance (ASSD): Determines the average 
% difference between the surface of the segmented object and the reference 
% in 3D. After the border voxels of segmentation and reference are 
% determined, those voxels that have at least one neighbor from a 
% predefined neighborhood that does not belong to the object are collected.
% https://chaos.grand-challenge.org/Evaluation/

function [ASSD, MSSD] = compute_assd(im_bw, im_gt_bw, mask, min_xy, max_xy)
%compute_similarity Compute the DICE similarity metric between input
%images and a defined region of interest.
% https://www.mathworks.com/help/images/ref/dice.html

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

% Extract border voxels
im_gt_bw_border = im_bin & ~imerode(im_bin, strel('sphere', 2));
im_bw_border = im_gt_bin & ~imerode(im_gt_bin, strel('sphere', 2));

% Show the images
figure; 
imshow(imfuse(im_bw_border * 100, im_gt_bw_border * 200,'blend','Scaling','joint'))
title('Image contour pairs');

[x1,y1,z1]=ind2sub(size(im_bw_border),find(im_bw_border==1));
BorderVoxelsRef=[x1,y1,z1];

[x2,y2,z2]=ind2sub(size(im_gt_bw_border),find(im_gt_bw_border==1));
BorderVoxelsResult=[x2,y2,z2];

if ~isempty(BorderVoxelsRef) && ~isempty(BorderVoxelsResult)
    % Distance between border voxels
    MdlKDTResult = KDTreeSearcher(BorderVoxelsResult);
    [~,distIndex1] = knnsearch(MdlKDTResult,BorderVoxelsRef);
    distIndex1=distIndex1';
    
    MdlKDTRef = KDTreeSearcher(BorderVoxelsRef);
    [~,distIndex2] = knnsearch(MdlKDTRef,BorderVoxelsResult);
    distIndex2=distIndex2';
    
    % Metrics
    ASSD=(sum(distIndex1)+sum(distIndex2))/(size(distIndex1,2)+size(distIndex2,2));
    MSSD=max([distIndex1,distIndex2]);
else
    [ASSD, MSSD]=deal(dist(size(Vref),0)); % Highest possible distance if any problem occurs
end

end
    