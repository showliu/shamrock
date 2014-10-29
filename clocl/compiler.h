/*
 * Copyright (c) 2011, Denis Steckelmacher <steckdenis@yahoo.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file compiler.h
 * \brief Compiler wrapped around Clang
 */

#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <string>

#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm
{
    class MemoryBuffer;
    class Module;
}

namespace clang
{
    class TextDiagnosticPrinter;
}


/**
 * \brief Compiler using Clang
 * 
 * This class builds a Clang instance, runs it and then retains compilation logs
 * and produced data.
 */
class Compiler
{
    public:
        /**
         * \brief Constructor
         */
        Compiler();
        ~Compiler();

        /**
         * \brief Compile \p source to produce a LLVM module
         * \param options options given to the compiler, described in the OpenCL spec
         * \param source source to be compiled
         * \return true if the compilation is successful, false otherwise
         * \sa module()
         * \sa log()
         */
        bool compile(const std::string &options, llvm::MemoryBuffer *source,
                     std::string filename);

        /**
         * \brief Compilation log
         * \note \c appendLog() can also be used to append custom info at the end
         *       of the log, for instance to keep compilation and linking logs
         *       in the same place
         * \return log
         */
        const std::string &log() const;

        /**
         * \brief Options given at \c compile()
         * \return options used during compilation
         */
        const std::string &options() const;

        /**
         * \brief Optimization enabled
         * \return true if -cl-opt-disable was given in the options, false otherwise
         */
        bool optimize() const;

        /**
         * \brief LLVM module generated
         * \return LLVM module generated by the compilation, 0 if an error occured
         */
        llvm::Module *module() const;

        /**
         * \brief Append a string to the log
         * 
         * This function can be used to append linking or code-gen logs to the
         * internal compilation log kept by this class
         * 
         * \param log log to be appended
         */
        void appendLog(const std::string &log);

    private:
        clang::CompilerInstance p_compiler;
        llvm::Module *p_module;
        bool p_optimize;

        std::string p_log, p_options;
        llvm::raw_string_ostream p_log_stream;
        clang::TextDiagnosticPrinter *p_log_printer;
};

#endif
