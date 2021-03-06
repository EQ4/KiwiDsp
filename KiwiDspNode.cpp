/*
 ==============================================================================
 
 This file is part of the KIWI library.
 Copyright (c) 2014 Pierre Guillot & Eliott Paris.
 
 Permission is granted to use this software under the terms of either:
 a) the GPL v2 (or any later version)
 b) the Affero GPL v3
 
 Details of these licenses can be found at: www.gnu.org/licenses
 
 KIWI is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 
 ------------------------------------------------------------------------------
 
 To release a closed-source product which uses KIWI, contact : guillotpierre6@gmail.com
 
 ==============================================================================
*/

#include "KiwiDspNode.h"
#include "KiwiDspChain.h"
#include "KiwiDspContext.h"

namespace Kiwi
{
    // ================================================================================ //
    //                                      DSP NODE                                    //
    // ================================================================================ //
    
    DspNode::DspNode(sDspChain chain) noexcept :
    m_chain(chain),
    m_nins(0),
    m_sample_ins(nullptr),
    m_nouts(0),
    m_sample_outs(nullptr),
    m_samplerate(0),
    m_vectorsize(0),
    m_inplace(true),
    m_running(false)
    {
        for(ulong i = 0; i < getNumberOfInputs(); i++)
        {
            m_inputs.push_back(make_shared<DspInput>(i));
        }
        for(ulong i = 0; i < getNumberOfOutputs(); i++)
        {
            m_outputs.push_back(make_shared<DspOutput>(i));
        }
    }
    
    DspNode::~DspNode() noexcept
    {
        stop();
        if(m_sample_ins)
        {
            delete [] m_sample_ins;
        }
        if(m_sample_outs)
        {
            delete [] m_sample_outs;
        }
        m_inputs.clear();
        m_outputs.clear();
    }
    
    sDspContext DspNode::getContext() const noexcept
    {
        sDspChain chain = getChain();
        if(chain)
        {
            return chain->getContext();
        }
        else
        {
            return nullptr;
        }
    }
    
    sDspDeviceManager DspNode::getDeviceManager() const noexcept
    {
        sDspContext context = getContext();
        if(context)
        {
            return context->getDeviceManager();
        }
        else
        {
            return nullptr;
        }
    }
    
    void DspNode::setNumberOfInlets(const ulong nins) throw(DspError&)
    {
        bool state = false;
        if(m_running)
        {
            sDspChain chain = getChain();
            if(chain)
            {
                try
                {
                    state = chain->suspend();
                }
                catch(DspError& e)
                {
                    throw e;
                }
            }
            else
            {
                stop();
            }
        }
        m_nins = nins;
        if(m_sample_ins)
        {
            delete [] m_sample_ins;
        }
        m_sample_ins = new sample*[m_nins];
        sDspChain chain = getChain();
        if(chain)
        {
            try
            {
                chain->resume(state);
            }
            catch(DspError& e)
            {
                throw e;
            }
        }
    }
    

    void DspNode::setNumberOfOutlets(const ulong nouts) throw(DspError&)
    {
        bool state = false;
        if(m_running)
        {
            sDspChain chain = getChain();
            if(chain)
            {
                try
                {
                    state = chain->suspend();
                }
                catch(DspError& e)
                {
                    throw e;
                }
            }
            else
            {
                stop();
            }
        }
        m_nouts = nouts;
        if(m_sample_outs)
        {
            delete [] m_sample_outs;
        }
        m_sample_outs = new sample*[m_nouts];
        sDspChain chain = getChain();
        if(chain)
        {
            try
            {
                chain->resume(state);
            }
            catch(DspError& e)
            {
                throw e;
            }
        }
    }
    
    void DspNode::addInput(sDspNode node, const ulong index)
    {
        if(index < (ulong)m_inputs.size())
        {
            m_inputs[index]->add(node);
        }
    }
    
    void DspNode::addOutput(sDspNode node, const ulong index)
    {
        if(index < (ulong)m_outputs.size())
        {
            m_outputs[index]->add(node);
        }
    }
    
    void DspNode::removeInput(sDspNode node, const ulong index)
    {
        if(index < (ulong)m_outputs.size())
        {
            m_inputs[index]->remove(node);
        }
    }
    
    void DspNode::removeOutput(sDspNode node, const ulong index)
    {
        if(index < (ulong)m_outputs.size())
        {
            m_outputs[index]->remove(node);
        }
    }
    
    bool DspNode::isInputConnected(const ulong index) const noexcept
    {
        return !m_inputs[index]->empty();
    }
    
    bool DspNode::isOutputConnected(const ulong index) const noexcept
    {
        return !m_outputs[index]->empty();
    }
    
    void DspNode::setInplace(const bool status) noexcept
    {
        m_inplace = status;
    }
    
    void DspNode::shouldPerform(const bool status) noexcept
    {
        m_running = status;
    }
    
    void DspNode::start() throw(DspError&)
    {
        sDspChain chain = getChain();
        if(chain)
        {
            stop();

            m_samplerate = chain->getSampleRate();
            m_vectorsize = chain->getVectorSize();
            
            for(ulong i = 0; i < getNumberOfInputs(); i++)
            {
                try
                {
                    m_inputs[i]->start(shared_from_this());
                }
                catch(DspError& e)
                {
                    m_running = false;
                    throw e;
                }
                m_sample_ins[i] = m_inputs[i]->getVector();
            }
            for(ulong i = 0; i < getNumberOfOutputs(); i++)
            {
                try
                {
                    m_outputs[i]->start(shared_from_this());
                }
                catch(DspError& e)
                {
                    m_running = false;
                    throw e;
                }
                
                m_sample_outs[i] = m_outputs[i]->getVector();
            }
            
            prepare();
        }
    }
    
    void DspNode::stop()
    {
        if(m_running)
        {
            m_running = false;
            release();
            for(ulong i = 0; i < getNumberOfInputs(); i++)
            {
                m_inputs[i]->clear();
            }
            for(ulong i = 0; i < getNumberOfOutputs(); i++)
            {
                m_outputs[i]->clear();
            }
        }
    }
}
















