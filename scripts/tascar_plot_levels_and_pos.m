function tascar_plot_levels_and_pos( data )
figure
Lmin = 35;
degdb = 1;
colors = colormap('jet');
kCol = 1;
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
            az = atan2(obj.y(idx,kSound),obj.x(idx,kSound))*180/pi;
            patch([t;t(end:-1:1)],...
                  [az+degdb*L;az(end:-1:1)-degdb*L(end:-1:1)],...
                  colors(kCol,:),...
                  'EdgeColor',0.5*colors(kCol,:),...
                  'FaceAlpha',0.7);
            hold on;
            plot(t,az,'-','linewidth',2,'Color',0.5*colors(kCol,:));
        end
        [Lmax,imax] = max(max(obj.lev,[],2));
        Lmax = Lmax-Lmin;
        if Lmax > 0
            az = atan2(obj.y(:,1),obj.x(:,1))*180/pi;
            text(obj.t(imax),az(imax)+degdb*(Lmax+1),objects{kObj},...
                 'Interpreter','none','Color',0.3*colors(kCol,:),...
                 'Fontweight','bold');
            kCol = mod(kCol + floor(size(colors,1)/2.1),size(colors,1))+1;
        end
    end
end
xlabel('time / seconds');
ylabel('azmiuth / degree');