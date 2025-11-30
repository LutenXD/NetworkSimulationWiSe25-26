@echo off
REM Script to run the M/M/1 parameter study on Windows
REM Usage: run_parameter_study.bat

echo Starting M/M/1 Parameter Study
echo Configuration: β_producer = 1s to 10s (0.1s steps), β_service = 1s
echo Simulation time: 600s with 100s warmup
echo Repetitions: 5 per parameter value
echo Total runs: 90 parameter values × 5 repetitions = 450 runs
echo.

REM Run the parameter study
opp_runall -j4 mm1_queue.exe -u Cmdenv -c ParameterStudy -n .:

echo.
echo Parameter study completed!
echo Results are saved in:
echo   - results/ directory for scalar and vector files
echo   - Use opp_scavetool to analyze the results
echo.
echo Example analysis commands:
echo   opp_scavetool query results/*.sca
echo   opp_scavetool export -f CSV -o results.csv results/*.sca
pause