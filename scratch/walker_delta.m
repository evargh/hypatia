%% BUILT-IN WALKER DELTA SIMULATIONS
% parameters are entered here
% currently, these are starlink's parameters. In case certain results
% aren't appearing if these parameters change, be sure to adjust ylim in
% the plots

% number of shells
NUM_SHELLS = 5;

% altitude of orbit
ALTITUDE_M = [550, 1110, 1130, 1275, 1325]*1e3;

% number of orbits
ORBIT_NUM = [72, 32, 8, 5, 6];

% satellites per orbit
SATSPERORBIT_NUM = [22, 50, 50, 75, 75];

% inclination of orbit
INCLINATION_DEGREES = [53, 53.8, 74, 81, 70];

% orbital phases
ORBIT_PHASES_DEGREES = [0, 0, 0, 0, 0];

%% check 1-day period, sampling every 30 seconds
startTime = datetime(2020,5,5,0,0,0);
stopTime = startTime + days(1);
sampleTime = 120;
sat_scenario = satelliteScenario(startTime, stopTime, sampleTime);

EARTH_RADIUS_KM = 6372e3;

sats = cell(NUM_SHELLS,1);
orbits = cell(NUM_SHELLS,1);
positions_cell = cell(NUM_SHELLS,1);
angles_cell = cell(NUM_SHELLS,1);
%% create and reshape the array of satellites into a matrix where each column is a different orbit
for shell=1:NUM_SHELLS
    sats{shell} = walkerDelta(sat_scenario, ALTITUDE_M(shell)+EARTH_RADIUS_KM, INCLINATION_DEGREES(shell), ORBIT_NUM(shell) * SATSPERORBIT_NUM(shell), ...
        ORBIT_NUM(shell), ORBIT_PHASES_DEGREES(shell), Name="S1");
    orbits{shell} = reshape(sats{shell}, [SATSPERORBIT_NUM(shell), ORBIT_NUM(shell)]);
    % generate latitudes from this library's coordinate system
    positions_cell{shell} = states(orbits{shell}(1,1));
    angles_cell{shell} = atan(positions_cell{shell}(3,:)./sqrt(positions_cell{shell}(2,:).^2 + positions_cell{shell}(1,:).^2))*180/pi;
end
angles = cell2mat(angles_cell);
%% generate sets of distances over time and take their minima
different_orbit_satellites = cell(NUM_SHELLS,1);
intraorbit_range_cell = cell(NUM_SHELLS,1);
sd_interorbit_range_cell = cell(NUM_SHELLS,1);
a_interorbit_range_cell = cell(NUM_SHELLS,1);
for shell=1:NUM_SHELLS
    different_orbit_satellites{shell} = sats{shell}(SATSPERORBIT_NUM(shell)+1:length(sats{shell}));
    [~, ~, intraorbit_range_cell{shell}] = aer(orbits{shell}(1,1), orbits{shell}(2,1));
    [~, ~, sd_interorbit_range_cell{shell}] = aer(orbits{shell}(1,1), orbits{shell}(1,2:ORBIT_NUM(shell)));
    sd_interorbit_range_cell{shell} = min(sd_interorbit_range_cell{shell});
    [~, ~, a_interorbit_range_cell{shell}] = aer(orbits{shell}(1,1), different_orbit_satellites{shell});
    a_interorbit_range_cell{shell} = min(a_interorbit_range_cell{shell});
end
% these matrices contain minimum distances for each time unit
intraorbit_range = cell2mat(intraorbit_range_cell);
same_direction_interorbit_range = cell2mat(sd_interorbit_range_cell);
all_interorbit_range = cell2mat(a_interorbit_range_cell);

%%
intraorbit_delay_ms = intraorbit_range/3e5;
same_direction_interorbit_delay_ms = same_direction_interorbit_range/3e5;
all_interorbit_delay_ms = all_interorbit_range/3e5;

%% Plot minimum distances over time
figure
for shell=1:NUM_SHELLS
    subplot(NUM_SHELLS,1,shell);
    hold on
    plot(intraorbit_delay_ms(shell,:))
    plot(same_direction_interorbit_delay_ms(shell,:))
    plot(all_interorbit_delay_ms(shell,:))
    legend('minimum intraorbit delay', 'minimum same-direction interorbit delay', 'minimum interorbit delay')
    title(sprintf('Inter-Satellite Delays for shell %d', shell))
    xlabel('Time (over 1 day)')
    ylabel('Delay (ms)')
    ylim([0 30]);
    hold off
end

%% Plot minimum distances over latitude
figure
for shell=1:NUM_SHELLS
    subplot(NUM_SHELLS,1,shell);
    hold on
    scatter(abs(angles(shell,:)), same_direction_interorbit_delay_ms(shell,:),".")
    scatter(abs(angles(shell,:)), all_interorbit_delay_ms(shell,:),".")
    title(sprintf('Inter-Orbit Satellite Delays for Shell %d', shell))
    xlabel('Latitude')
    ylabel('Delay (ms)')
    legend('minimum same-direction interorbit delay', 'minimum interorbit delay')
    ylim([0 30]);
    hold off
end

%% Plot statistics of distances
% this plot is potentially misleading, because in lower shells more hops
% are required. note the displayed values in the console that show coefficient of
% variation
figure
for shell=1:NUM_SHELLS
    subplot(NUM_SHELLS,1,shell);
    boxplot([transpose(same_direction_interorbit_delay_ms(shell,:)), transpose(all_interorbit_delay_ms(shell,:))], 'Labels',{'minimum same-direction interorbit delay', 'minimum interorbit delay'})
    ylabel('Delay (ms)')
    title(sprintf('Single-Link Delay Statistics for Shell %d', shell))
    ylim([0 30]);
    sd_stdev = sqrt(var(same_direction_interorbit_delay_ms));
    a_stdev = sqrt(var(all_interorbit_delay_ms));
    sd_mean = mean(same_direction_interorbit_delay_ms);
    a_mean = mean(all_interorbit_delay_ms);
    fprintf('Shell %d Coefficients of Variation for Same Direction and All Direction, respectively\n', shell)
    sd_coeffvar = sd_stdev/sd_mean
    a_coeffvar = a_stdev/a_mean
end


%% Visualize (if desired)
%satelliteScenarioViewer(sat_scenario, ShowDetails=true);