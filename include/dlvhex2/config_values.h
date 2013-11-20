/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2013 Andreas Humenberger
 * 
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */


/**
 * @file   config_values.h
 * @author Andreas Humenberger
 *
 * @brief Config key values for class Configuration.
 *
 *
 */

#ifndef CONFIG_VALUES_H
#define CONFIG_VALUES_H

#define CFG_HT_MODELS			"ht_models"

#define CFG_MODEL_BUILDER		"model_builder"

enum ModelBuilder
{
	ModelBuilder_None = 0,
	ModelBuilder_Offline,
	ModelBuilder_Online,
};

#define CFG_EVAL_HEURISTIC		"eval_heuristic"
#define CFG_EVAL_HEURISTIC_ARG	"eval_heuristic_arg"

enum EvalHeuristic
{
	Eval_None = 0,
	Eval_Easy,
	Eval_Greedy,
	Eval_OldDlvhex,
	Eval_Trivial,
	Eval_Monolithic,
	Eval_FromFile,
	Eval_ASP,
	Eval_FromHEXSourcecode,
};

#endif
