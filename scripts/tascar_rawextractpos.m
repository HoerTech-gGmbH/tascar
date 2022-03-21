function tascar_rawextractpos
d = dir('*.raw');
names = {};
for k=1:numel(d)
    data = load(d(k).name);
    if numel(data) == 9
        names{end+1} = d(k).name(1:end-4);
    end
end
kunique = numel(names{1});
for k=2:numel(names)
    lkunique = 0;
    for c=1:min(kunique,numel(names{k}))
        if names{k}(c) == names{1}(c)
            lkunique = c;
        end
    end
    kunique = lkunique;
end
for k=1:numel(d)
    data = load(d(k).name);
    if numel(data) == 9
        data = reshape(data,[3,3]);
        k0 = 0;
        k1 = 0;
        k2 = 0;
        rz = 0;
        for vec=1:3
            dvec = data-repmat(data(:,vec),[1,3]);
            idx = setdiff(1:3,vec);
            dvec = dvec(:,idx);
            l = sqrt(sum(dvec.^2,1));
            if abs(dvec(:,1)' * dvec(:,2)) < 1e-5
                k0 = vec;
                k3 = 0;
                if l(1) > l(2)
                    k2 = idx(2);
                    k3 = 1;
                else
                    k2 = idx(1);
                    k3 = 2;
                end
                k1 = vec;
                rz = 0.01*round(100*180*atan2(dvec(2,k3),dvec(1,k3))/pi);
            end
        end
        data = 0.001*round(1000*data);
        name = d(k).name(kunique+1:end-4);
        if strfind(name,'source')==1
            disp(sprintf(['<source name="%s">'...
                          '<position>0 %g %g %g</position>'...
                          '<orientation>0 %g 0 0</orientation>'...
                          '<sound/></source>'],...
                         name(8:end),data(1,k1),data(2,k1),data(3,k1),rz));
        elseif strfind(name,'receiver')==1
            disp(sprintf(['<receiver name="%s">'...
                          '<position>0 %g %g %g</position>' ...
                          '<orientation>0 %g 0 0"</orientation>'...
                          '</receiver>'],...
                         name(10:end),data(1,k1),data(2,k1),data(3,k1),rz));
        else 
            disp(sprintf(['<object name="%s">'...
                          '<position>0 %g %g %g</position>' ...
                          '<orientation>0 %g 0 0</orientation>'...
                         '</object>'],...
                         name,data(1,k1),data(2,k1),data(3,k1),rz));
        end
    end
end
