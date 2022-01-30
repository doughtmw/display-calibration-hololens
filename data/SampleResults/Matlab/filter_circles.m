% https://stackoverflow.com/questions/29876452/how-can-i-remove-overlaping-circles-after-hough-transform-segmentation
function [centers_use, radii_use, metric_use] = filter_circles(centers, radii, metric)
idx_mask = ones(size(radii));

min_dist = 1; % relative value. Tweak this if slight overlap is OK.
for i = 2:length(radii)
    cur_cent = centers(i, :);
    for j = 1:i-1
        other_cent = centers(j,:);
        x_dist = other_cent(1) - cur_cent(1);
        y_dist = other_cent(2) - cur_cent(2);
        if sqrt(x_dist^2+y_dist^2) < min_dist*(radii(i) + radii(j)) && idx_mask(j) == 1
            idx_mask(i) = 0;
            break
        end
    end
end
%%

idx_mask = logical(idx_mask);
centers_use = centers(idx_mask, :);
radii_use = radii(idx_mask, :);
metric_use = metric(idx_mask, :);
end

