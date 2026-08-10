\# LLM-LOG



\## Model and tool



GPT-5.6 Luna via ChatGPT.



\## What code was provided to the LLM?



The complete `snake.cpp` file was provided as context.



\## Assignment document provided?



Yes. The assignment/Part D instructions were provided to the LLM.



\## Purpose of LLM use



The LLM was used to reason about the existing single-player Snake implementation and help implement multiplayer support with a second snake/player.



\## Prompts to working code



6 Number of prompts



\## Summary of the requested change



Add a second player to the existing Snake game while preserving the original Player 1 controls.



Player 1:

\- Arrow keys



Player 2:

\- W/A/S/D



The implementation also needed:

\- separate scores,

\- collision handling between players,

\- food spawning that accounts for both snakes,

\- obstacle spawning that accounts for both snakes,

\- rendering of both snakes,

\- player-specific game-over information.



\## Verification



The code was reviewed through Git diff and repository state.

