function tascar_plot_levels_and_pos( data )
figure
Lmin = 35;
degdb = 1;
colors = colormap('jet');
kCol = 1;
pos_az = [];
for kScene=1:numel(data)
    sData = data{kScene};
    objects = fieldnames(sData)';
    for kObj=1:numel(objects)
        obj = sData.(objects{kObj});
        for kSound=1:size(obj.lev,2)
            L = max(0,obj.lev(:,kSound)-Lmin);
            imin = find(obj.lev(:,kSound) > Lmin,1,'first');
            imax = find(obj.lev(:,kSound) > Lmin,1,'last');
            idx = imin:imax;
            %            L(obj.lev(:,kSound)<Lmin) = nan;
            t = obj.t(idx);
            L = L(idx);
            az = unwrap(atan2(obj.y(idx,kSound),obj.x(idx,kSound)))*180/pi;
            az_med = 360*round(median(az)/360);
            az = az-az_med;
            patch([t;t(end:-1:1)],...
                [az+degdb*L;az(end:-1:1)-degdb*L(end:-1:1)],...
                colors(kCol,:),...
                'EdgeColor',0.5*colors(kCol,:),...
                'FaceAlpha',0.7);
            hold on;
            plot(t,az,'-','linewidth',2,'Color',0.5*colors(kCol,:));
        end
        kCol = mod(kCol + floor(size(colors,1)/2.1),size(colors,1))+1;
    end
end
kCol = 1;
for kScene=1:numel(data)
    sData = data{kScene};
    objects = fieldnames(sData)';
    for kObj=1:numel(objects)
        obj = sData.(objects{kObj});
        [Lmax,~] = max(max(obj.lev,[],2));
        Lmax = Lmax-Lmin;
        imax = find(max(obj.lev,[],2) > Lmin,1,'last');
        if Lmax > 0
            kSound = 1;
            az = unwrap(atan2(obj.y(:,kSound),obj.x(:,kSound)))*180/pi;
            az_med = 360*round(median(az)/360);
            az = az-az_med;
            if min(abs(pos_az-az(imax))) < 5
                pos_az = [pos_az az(imax)+6];
            elseif strcmp(objects{kObj},'tv')
                pos_az = [pos_az az(imax)-10];
            else
                pos_az = [pos_az az(imax)];
            end
            text(obj.t(imax),pos_az(end),objects{kObj},...
                'Interpreter','none','Color',0.3*colors(kCol,:),...
                'Fontweight','bold','Fontsize',14);
        end
        kCol = mod(kCol + floor(size(colors,1)/2.1),size(colors,1))+1;
    end
end
xlabel('time / seconds');
ylabel('azmiuth / degree');