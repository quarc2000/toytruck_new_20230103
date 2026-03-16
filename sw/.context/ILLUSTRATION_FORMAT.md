# Illustration Format

## Purpose
This file defines the default visual style to use when generating illustration prompts for this project.

The goal is consistency across:
- architecture illustrations
- context-architecture illustrations
- variable-model illustrations
- truck system overviews
- formal documentation boards

## Default Style
- formal
- modern
- elegant
- technical
- minimal clutter
- documentation-quality
- light background
- restrained engineering color palette
- strong visual hierarchy
- landscape layout preferred

## Tone
- serious engineering documentation
- clean internal architecture board
- suitable for formal project documentation
- not playful
- not cartoonish
- not childish
- not sci-fi
- not marketing art
- not a generic software flowchart

## Composition Preferences
- use clear sectioning
- use a subtle grid or well-aligned layout
- prefer a few strong grouped structures over many small floating elements
- keep the main concept visually obvious
- use arrows only when they add meaning
- use dashed connections for future or deferred paths
- use solid connections for current or verified paths

## Text Policy
- keep text to a minimum
- only include text that is necessary to understand the illustration
- avoid dense paragraphs inside the image
- avoid too many labels
- prefer a few strong headings and short labels over verbose callouts
- because current LLM image systems are poor at rendering dense text, the prompt should explicitly request minimal text

## Preferred Visual Qualities
- excellent readability at a glance
- minimal clutter
- strong information hierarchy
- elegant spacing
- clear grouping
- a polished architecture-poster feel

## Project-Specific Visual Guidance
- if the truck itself is shown, it should still look like a toy truck, not a generic robot platform
- if internals are shown, prefer cutaway, x-ray, translucent, or simplified exploded views
- when illustrating software architecture, favor abstraction over excessive low-level detail unless the request explicitly asks for detail
- when illustrating variables or context files, emphasize structure and relationships more than decorative effects

## Prompt Guidance
When creating an illustration prompt for this project, prefer wording such as:
- formal
- visually refined
- elegant
- clean
- technically precise
- documentation-quality
- minimal text
- light background
- restrained palette
- strong hierarchy

## Anti-Patterns To Avoid
- crowded diagrams
- too much embedded text
- playful infographic style
- glossy product advertisement style
- dark futuristic UI look
- random icons without architectural meaning
- decorative 3D effects that reduce clarity
