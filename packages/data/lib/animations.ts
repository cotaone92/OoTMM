import { default as DATA } from '../dist/lib/data-link-animations.json';
import type { Game } from './game';

export type CustomAnimation = {
  name: string;
  home_game: Game;
  header_offset: number;
  frame_data_offset: number;
  /** Number of animated limbs in each frame; defaults to the standard Link skeleton. */
  limb_count?: number;
  frame_count?: number;
  frame_data_size?: number;
};

export const DATA_ANIMATIONS = DATA.animations as CustomAnimation[];
