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
    @file Model_Power.hpp
    @brief Contains a mathematical model for calculating the power usage of the
    target program.
    @author Scott Egerton
    @date 2017-2018
    @copyright GNU Affero General Public License Version 3+
*/

#ifndef MODEL_POWER_HPP
#define MODEL_POWER_HPP

#include <bitset>   // for bitset
#include <cstdint>  // for size_t
#include <string>   // for string
#include <utility>  // for pair

#include "Assembly_Instruction.hpp"
#include "Coefficients.hpp"
#include "Execution.hpp"

#include "Model.hpp"                   // for Model
#include "Model_Factory_Register.hpp"  // for Model_Factory_Register

namespace ELMO2
{
namespace Internal
{
//! @class Model_Power
//! @class Model_Hamming_Weight
//! @brief This derived class contains a specific implementation of a
//! mathematical model to calculate the traces for a target program. It is
//! designed as a template allowing new models to be added with ease.
//! Deriving from Model_Factory_Register as well will automatically register
//! this class within the factory class.
class Model_Power : public virtual ELMO2::Internal::Model,
                    public ELMO2::Internal::Model_Factory_Register<
                        ELMO2::Internal::Model_Power>
{
private:
    class Instruction_Terms_Helper
    {
    protected:
        static std::size_t calculate_interactions(const std::bitset<32>& p_term)
        {
            std::size_t result = 0;

            // Loop through all possible combinations of i and j where i and j
            // are never the same.
            for (std::size_t term_1 = 0; term_1 < 32; ++term_1)
            {
                for (std::size_t term_2 = term_1 + 1; term_2 < 32; ++term_2)
                {
                    result += p_term[term_1] *
                              p_term[term_2];  // TODO: What is this doing?
                                               // - Logical AND but why?
                }
            }
            return result;
        }

        //! @brief Ensures this is a pure virtual class, only able to be
        //! inherited from.
        // virtual ~Instruction_Terms_Helper() = 0;
    };

    //! @todo document
    // Used to store intermediate terms needed in leakage calculations, that are
    // related to a specific instruction. This exists for the simple reason of
    // saving the time recalculating the data.
    // TODO: This should inherit from Assembly_Instruction
    struct Assembly_Instruction_Power
        : public ELMO2::Internal::Assembly_Instruction,
          public Instruction_Terms_Helper
    {
        // Move constructed
        Assembly_Instruction_Power(
            const ELMO2::Internal::Assembly_Instruction&& p_instruction,
            const std::size_t p_operand_1,
            const std::size_t p_operand_2)  // TODO: Encode Operand value into
                                            // Assembly_Instruction
            : m_opcode(std::move(
                  p_instruction.m_opcode)),  // TODO: should move be exchange?
              m_omerands(std::move(p_instruction.m_operands)),
              Operand_1_Bit_Interactions(calculate_interactions(p_operand_1)),
              Operand_2_Bit_Interactions(calculate_interactions(p_operand_2))
        {
        }

        // Does this need to be stored? -
        // Saves recalculating it
        //
        // TODO: MOVE OPERANDS UP TO
        // ASSEMBLY_INSTRUCTION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        const std::uint32_t Operand_1;
        const std::uint32_t Operand_2;
        const std::size_t Operand_1_Bit_Interactions;
        const std::size_t Operand_2_Bit_Interactions;
    };

    //! @todo: document
    // This stores intermediate terms that are related to the interactions
    // between two different instructions. This exists for the simple reason of
    // saving the time recalculating the data.
    struct Instruction_Terms_Interactions : public Instruction_Terms_Helper
    {
        Instruction_Terms_Interactions(
            const ELMO2::Internal::Model_Power::Assembly_Instruction_Power&
                p_instruction_1,
            const ELMO2::Internal::Model_Power::Assembly_Instruction_Power&
                p_instruction_2)
            : Operand_1_Bit_Flip(calculate_bitflips(p_instruction_1.Operand_1,
                                                    p_instruction_2.Operand_2)),
              Operand_2_Bit_Flip(calculate_bitflips(p_instruction_1.Operand_1,
                                                    p_instruction_2.Operand_2)),
              Bit_Flip1_Bit_Interactions(
                  calculate_interactions(Operand_1_Bit_Flip)),
              Bit_Flip2_Bit_Interactions(
                  calculate_interactions(Operand_2_Bit_Flip))
        {
        }
        // TODO: I think only the hw and hd need to be stored?
        // BitFlip is 32 bools indicating whether a bitflip has occurred between
        // current and previous instruction. BitFlip
        const std::bitset<32> Operand_1_Bit_Flip;
        const std::bitset<32> Operand_2_Bit_Flip;
        const std::size_t Bit_Flip1_Bit_Interactions;
        const std::size_t Bit_Flip2_Bit_Interactions;

    private:
        static const std::bitset<32>
        calculate_bitflips(const std::bitset<32>& p_instruction_1_operand,
                           const std::bitset<32>& p_instruction_2_operand)
        {
            std::bitset<32> result;
            for (std::size_t i = 0; i < result.size(); ++i)
            {
                result[i] =
                    p_instruction_1_operand[i] != p_instruction_2_operand[i];
            }
            return result;
        }
    };

    static const std::unordered_set<std::string> m_required_interaction_terms;

    //! @brief Retrieves a list of the interaction terms that are used within
    //! the model. These must be provided by the Coefficients in order for
    //! the model to function.
    //! @returns The list of interaction terms used within the model.
    const std::unordered_set<std::string>&
    Get_Interaction_Terms() const override
    {
        return m_required_interaction_terms;
    }

    const ELMO2::Internal::Model_Power::Assembly_Instruction_Power
    get_instruction_terms(const std::size_t& p_cycle) const
    {
        // Prevents trying to calculate the hamming weight of stalls and
        // flushes.
        // Currently stalls and flushes are stored as zeros in calculations.
        if (!m_execution.Is_Normal_State(p_cycle, "Execute"))
        {
            return ELMO2::Internal::Model_Power::Assembly_Instruction_Power(
                m_execution.Get_Instruction(p_cycle, "Execute"), 0, 0);
        }

        // Retrieves what is in the "Execute" pipeline stage at clock cycle "i".
        const auto& instruction =
            m_execution.Get_Instruction(p_cycle, "Execute");

        // Add the next set of operands.
        return ELMO2::Internal::Model_Power::Assembly_Instruction_Power(
            m_execution.Get_Instruction(p_cycle, "Execute"),
            m_execution.Get_Operand_Value(p_cycle, instruction, 1),
            m_execution.Get_Operand_Value(p_cycle, instruction, 2));
    }

    const double calculate_term(
        const std::string& p_opcode,
        const std::string& p_term_name,
        const double p_instruction_term) const  // TODO: Why is this type
                                                // double?? What should it be?
    {
        // This is based off of what original elmo does to calculate an
        // individual term
        auto total = 0;
        for (const auto bit :
             m_coefficients.Get_Coefficients(p_opcode, p_term_name))
        {
            total += p_instruction_term * bit;
        }
        return total;
    }

public:
    //! @brief The constructor makes use of the base Model constructor to
    //! assist with initialisation of private member variables.
    Model_Power(const ELMO2::Internal::Execution& p_execution,
                const ELMO2::Internal::Coefficients& p_coefficients)
        : ELMO2::Internal::Model(p_execution, p_coefficients)
    {
        // This statement registers this class in the factory, allowing
        // access from elsewhere. Do not delete this or else this class will
        // not appear in the factory. If you wish to make this class
        // inaccessible, a better method would be to remove the
        // corresponding cpp file from the build script. This is required to
        // be "used" somewhere in order to prevent the compiler from
        // optimising it away, thus preventing self registration.
        // Section 6.6.4.1, point 2 of the linked document states that this
        // statement will not be optimised away.
        // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4713.pdf
        // The void cast does nothing functionally but prevents the compiler
        // warning about an unused result.
        (void)m_is_registered;

        // TODO: This can be moved up into Model if CTRP is used. CTRP can
        // work if a non template interface class is introduced.
        if (!this->Check_Interaction_Terms())
        {
            throw std::logic_error(
                "Model was not provided with correct "
                "interaction terms by the Coefficients file.");
        }
    }

    //! @brief This function contains the mathematical calculations that
    //! generate the power Traces.
    //! TODO: Improve this description with details of how elmo power model
    //! works.
    //! @returns The generated Traces for the target program
    const std::vector<float> Generate_Traces() const override;

    //! @brief Retrieves the name of this Model.
    //! @returns The name as a string.
    //! @note This is needed to ensure self registration in the factory works.
    //! The factory registration requires this as unique identifier.
    static const std::string Get_Name() { return "Power"; }
};
}  // namespace Internal
}  // namespace ELMO2

#endif
