/*
    This file is part of ELMO-2.

    ELMO-2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ELMO-2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with ELMO-2.  If not, see <http://www.gnu.org/licenses/>.
*/

/*!
    @todo Change this
    @file Model_Hamming_Weight.hpp
    @brief This file is a template. This can be used as a base for implementing
    a mathematical model for generating traces of a given target program.
    @author Scott Egerton
    @date 2017-2018
    @copyright GNU Affero General Public License Version 3+
*/

#include <cstddef>  // for uint8_t, size_t
#include <vector>   // for vector

#include <iostream>  // for temp debugging

#include "Model_Hamming_Weight.hpp"

#include "Assembly_Instruction.hpp"  // for Assembly_Instruction
#include "Execution.hpp"             // for Execution

//! The list of interaction terms used by this model in order to generate
//! traces.
const std::unordered_set<std::string>
    ELMO2::Internal::Model_Hamming_Weight::m_required_interaction_terms = {};

//! @brief This function contains the mathematical calculations that generate
//! the Traces.
//! @returns The generated Traces for the target program.
const std::vector<float>
ELMO2::Internal::Model_Hamming_Weight::Generate_Traces() const
{
    // TODO: If traces are serialised upon generation then do we need a
    // traces object?
    std::vector<float> traces;
    for (std::size_t i = 0; i < m_execution.Get_Cycle_Count(); ++i)
    {
        // Prevents trying to calculate the hamming weight of stalls and
        // flushes.
        if (!m_execution.Is_Normal_State(i, "Execute"))
        {
            // std::cout << "Abnormal state reached" << std::endl;
            // TODO: What to do in this situation?
            traces.push_back(0);
            continue;
        }

        // ******* DEBUG ONLY: TO BE REMOVED *******************

        /*
         *        // Retrieves what is in the "Execute" pipeline stage at clock
         * cycle "i". auto instruction = m_execution.Get_Instruction(i,
         * "Execute");
         *
         *                std::cout << "Cycle: " << i << std::endl;
         *
         *                std::cout << instruction.Get_Opcode();
         *
         *                for (const auto& operand : instruction.Get_Operands())
         *                {
         *                    // If the operand is a register look up the value
         * in that
         *                    // register,
         *                    // else treat it as a raw value.
         *                    if (!m_execution.Is_Register(operand))
         *                    {
         *                        std::cout << "," << operand;
         *                        continue;
         *                    }
         *                    std::cout << ",|" <<
         * m_execution.Get_Register_Value(i, operand)
         *                              << "|";
         *                }
         *                std::cout << std::endl;
         */
        // ******* DEBUG ONLY: TO BE REMOVED *******************

        // Calculates the Hamming weight of the first operand of the instruction
        // at clock cycle 'i' and appends it to the traces object.
        traces.push_back(hamming_weight(m_execution.Get_Operand_Value(
            i, m_execution.Get_Instruction(i, "Execute"), 1)));
    }
    /*
     *std::cout << "Number of traces: " << traces.Get_Number_Of_Traces()
     *          << std::endl
     *          << "Number of samples per trace: "
     *          << traces.Get_Number_Of_Samples_Per_Trace() << std::endl;
     */
    return traces;
}
