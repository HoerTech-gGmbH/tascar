function sData = tascar_perf_merge_data
    sDir = dir('results_*.m');
    csData = {sDir.name};
    cData = {};
    for k=1:numel(csData)
        cData{k} = feval(csData{k}(1:end-2));
    end
    sd = libsd();
    sData = sd.merge_addpar(cData{:});
    cpumodel = {};
    for k=1:numel(csData)
        s = cData{k};
        cpumodel{k} = strrep(strrep(s.cpumodel,' ',''),'CPU','');
    end
    sData.fields{1} = 'CPUmodel';
    sData.values{1} = cpumodel;
