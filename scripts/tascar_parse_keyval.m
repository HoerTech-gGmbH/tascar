function sCfg = tascar_parse_keyval( sCfg, sHelp, varargin )
  if (numel(varargin) == 1) && strcmp(varargin{1},'help')
    sOut = sprintf(' List of valid keys:\n');
    fields = fieldnames(sCfg);
    for k=1:numel(fields)
      field = fields{k};
      helps = '';
      if isfield(sHelp,field)
	helps = sHelp.(field);
      end
      defval = '?';
      if isnumeric(sCfg.(field)) || islogical(sCfg.(field))
	defval = mat2str(sCfg.(field));
      end
      if ischar(sCfg.(field))
	defval = sCfg.(field);
      end
      if iscellstr(sCfg.(field))
	defval = '{';
	if isempty(sCfg.(field))
	  defval(end+1) = ' ';
	end
	for ke=1:numel(sCfg.(field))
	  defval = sprintf('%s''%s'',',defval,sCfg.(field){ke});
	end
	defval(end) = '}';
      end
      sOut = sprintf('%s - %s:\n   %s\n   (default: %s)\n\n',...
		     sOut,field, ...
		     helps,defval);
    end
    disp(sOut);
    sCfg = [];
    return
  end
  if isempty(varargin)
    return;
  end
  if mod(numel(varargin),2) > 0
    error('Invalid (odd) number of input arguments.');
  end
  for k=1:2:numel(varargin)
    if ~ischar(varargin{k})
      error(sprintf('key %d is not a string.',k));
    end
    field = lower(varargin{k});
    if ~isfield(sCfg,field)
      error(sprintf('key %d (''%s'') is not a valid key.',k, ...
		    field));
    end
    sCfg.(field) = varargin{k+1};
  end
  