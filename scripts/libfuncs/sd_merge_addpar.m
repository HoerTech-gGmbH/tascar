function s = sd_merge_addpar( s, varargin )
% merge multiple data structures into one
%
% s : data structure
% varargin : one or more other data structures
%
% requires field names and parameter types to be equal for all
% structures.
  s.data = [ones(size(s.data,1),1),s.data];
  for k=1:numel(varargin)
    so = varargin{k};
    if ~isequal(s.fields,so.fields)
      error('field names mismatch');
    end
    if numel(s.values) ~= numel(so.values)
      error('different number of parameters');
    end
    for kf=1:numel(s.values)
      if ~isequal(class(s.values{kf}),class(so.values{kf}))
	error('parameter types mismatch');
      end
    end
    for kf=1:numel(s.values)
      so.data(:,kf) = so.data(:,kf) + numel(s.values{kf});
      s.values{kf} = [s.values{kf},so.values{kf}];
    end
    so.data = [(k+1)*ones(size(so.data,1),1),so.data];
    s.data = [s.data;so.data];
  end
  s.fields = [{'set'},s.fields];
  s.values = [{unique(s.data(:,1))},s.values];
  s = sd_compactval( s );
