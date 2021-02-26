function itbd = integrated_tone_burst_decay( irs )
% integrated_tone_burst_decay( irs )
%
% Method after: Schroeder, Manfred R. "New method of measuring
% reverberation time." The Journal of the Acoustical Society of
% America 37 (1965): 409.
%
% Author: Giso Grimm
% 4/2015
  itbd = zeros(size(irs));
  for k=1:size(irs,2)
    tburst = cumsum(irs(end:-1:1,k).^2);
    tburst = 10*log10(tburst(end:-1:1) ./ max(tburst));
    itbd(:,k) = tburst;
  end